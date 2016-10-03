#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/host_ops.h>
#include <asm/cpu.h>
#include <asm/thread_info.h>
#include <asm/unistd.h>
#include <asm/syscalls.h>

static void lkl_spin_lock(unsigned int *lock)
{
again:
	while (*lock != 0)
		;
	if (__sync_lock_test_and_set(lock, 1))
		goto again;
}

static void lkl_spin_unlock(unsigned int *lock)
{
	__sync_lock_release(lock);
}

/*
 * This structure is used to get access to the "LKL CPU" that allows us to run
 * Linux code. Because we have to deal with various synchronization requirements
 * between idle thread, system calls, interrupts, "reentrancy", CPU shutdown,
 * imbalance wake up (i.e. acquire the CPU from one thread and release it from
 * another), we can't use a simple synchronization mechanism such as (recursive)
 * mutex or semaphore. Instead, we use a spinlock and a bunch of status data
 * plus a semaphore.
 */
struct lkl_cpu {
	/* spinlock */
	unsigned int lock;
	/* shutdown in progress */
	bool shutdown;
	bool irqs_pending;
	/* no of threads waiting the CPU */
	unsigned int sleepers;
	/* no of times the current got the CPU */
	unsigned int count;
	/* current thread that owns the CPU */
	lkl_thread_t owner;
	/* semaphore for threads waiting the CPU */
	struct lkl_sem *sem;
	struct lkl_sem *idle_sem;
	struct lkl_sem *drain_sem;
	struct lkl_sem *shutdown_sem;
} cpu;

static int __lkl_cpu_try_get(void)
{
	lkl_thread_t self;

	if (cpu.shutdown)
		return -1;

	self = lkl_ops->thread_self();

	if (cpu.owner && cpu.owner != self)
		return 0;

	cpu.owner = self;
	cpu.count++;

	return 1;
}

static void __lkl_cpu_put(void)
{
	if (!cpu.count)
		lkl_bug("%s: unbalanced put\n", __func__);

	if (cpu.owner != lkl_ops->thread_self() && cpu.count > 1)
		lkl_bug("%s: trying to put reentrant owner\n", __func__);

	if (--cpu.count > 0)
		return;

	if (cpu.sleepers) {
		cpu.sleepers--;
		lkl_ops->sem_up(cpu.sem);
	}

	cpu.owner = 0;
}

int lkl_cpu_get(void)
{
	int ret;

	lkl_spin_lock(&cpu.lock);
	ret = __lkl_cpu_try_get();
	if (ret < 0) {
		lkl_spin_unlock(&cpu.lock);
		return ret;
	}

	while (ret == 0) {
		cpu.sleepers++;
		lkl_spin_unlock(&cpu.lock);
		lkl_ops->sem_down(cpu.sem);
		lkl_spin_lock(&cpu.lock);
		ret = __lkl_cpu_try_get();
	}

	if (cpu.shutdown)
		lkl_ops->sem_up(cpu.drain_sem);

	lkl_spin_unlock(&cpu.lock);

	return ret;
}

void lkl_cpu_set_irqs_pending(void)
{
	cpu.irqs_pending = true;
}

void lkl_cpu_put(void)
{
	lkl_spin_lock(&cpu.lock);
	while (cpu.irqs_pending) {
		cpu.irqs_pending = false;
		lkl_spin_unlock(&cpu.lock);
		run_irqs();
		lkl_spin_lock(&cpu.lock);
	}
	__lkl_cpu_put();
	lkl_spin_unlock(&cpu.lock);
}

int lkl_cpu_try_get_start(void)
{
	int ret;

	lkl_spin_lock(&cpu.lock);
	ret =  __lkl_cpu_try_get();
	if (ret)
		lkl_spin_unlock(&cpu.lock);
	return ret;
}

void lkl_cpu_try_get_stop(void)
{
	lkl_spin_unlock(&cpu.lock);
}

void lkl_cpu_shutdown(void)
{
	lkl_spin_lock(&cpu.lock);
	cpu.shutdown = true;
	lkl_spin_unlock(&cpu.lock);
}

void lkl_cpu_wait_shutdown(void)
{
	lkl_ops->sem_down(cpu.shutdown_sem);
	lkl_ops->sem_free(cpu.shutdown_sem);
}

void arch_cpu_idle(void)
{
	if (cpu.shutdown) {
		int wait, i;

		lkl_spin_lock(&cpu.lock);
		wait = cpu.sleepers;
		while (cpu.sleepers--)
			lkl_ops->sem_up(cpu.sem);
		lkl_spin_unlock(&cpu.lock);

		for(i = 0; i < wait; i++)
			lkl_ops->sem_down(cpu.drain_sem);

		lkl_ops->sem_free(cpu.drain_sem);
		lkl_ops->sem_free(cpu.idle_sem);
		lkl_ops->sem_up(cpu.shutdown_sem);

		lkl_ops->thread_exit();
	}

	/* enable irqs now to allow direct irqs to run */
	local_irq_enable();

	/* we may have run irqs and sofirqs, check if we need to reschedule */
	if (need_resched())
		return;

	lkl_cpu_put();

	lkl_ops->sem_down(cpu.idle_sem);

	lkl_cpu_get();

	/* since we've been waked up its highly likely we have pending irqs */
	local_irq_disable();
	local_irq_enable();
}

void lkl_cpu_wakeup(void)
{
	lkl_ops->sem_up(cpu.idle_sem);
}

int lkl_cpu_init(void)
{
	cpu.sem = lkl_ops->sem_alloc(0);
	if (!cpu.sem)
		return -ENOMEM;

	cpu.idle_sem = lkl_ops->sem_alloc(0);
	if (!cpu.idle_sem) {
		lkl_ops->sem_free(cpu.sem);
		return -ENOMEM;
	}

	cpu.drain_sem = lkl_ops->sem_alloc(0);
	if (!cpu.drain_sem) {
		lkl_ops->sem_free(cpu.sem);
		lkl_ops->sem_free(cpu.idle_sem);
		return -ENOMEM;
	}

	cpu.shutdown_sem = lkl_ops->sem_alloc(0);
	if (!cpu.shutdown_sem) {
		lkl_ops->sem_free(cpu.sem);
		lkl_ops->sem_free(cpu.idle_sem);
		lkl_ops->sem_free(cpu.drain_sem);
		return -ENOMEM;
	}

	return 0;
}
