#include <linux/kernel.h>
#include <asm/host_ops.h>
#include <asm/cpu.h>
#include <asm/thread_info.h>
#include <linux/tick.h>
#include <asm/unistd.h>
#include <asm/syscalls.h>

/*
 * The cpu_lock needs to be a semaphore since we may acquire the CPU
 * from one thread (host) and release it in another (idle, if the host
 * thread blocks during a system call).
 */
static struct lkl_sem *cpu_lock;

/*
 * A synchronization algorithm between lkl_cpu_get() / lkl_cpu_try_get() and cpu
 * shutdown (lkl_sys_halt) is needed because lkl_cpu_get() is called from
 * destructor threads which are not controlled by the user. Since the cpu
 * shutdown process frees the semaphore we need to avoid calling the sem_down
 * functions after lkl_sys_halt() has been issued.
 *
 * This also give us race condition avoidance between lkl_trigger_irq() /
 * lkl_syscall() and lkl_sys_halt() as well as protection against calling these
 * functions before lkl_start_kernel().
 *
 * An atomic counter is used to keep track of the number of pending sem_down
 * operations and allows the cpu shutdown process to wait for these operations
 * to complete before freeing the semaphore.
 *
 * The cpu shutdown process adds MAX_PENDING to this counter which allows the
 * pending CPU users to check if the shutdown process has started. This prevents
 * "late" sem_down calls on the now freed semaphore.
 *
 * This algorithm assumes that we never have more the MAX_PENDING simultaneous
 * calls into lkl_cpu_get().
 */
#define MAX_PENDING	(1UL << (sizeof(unsigned long) * 8 - 1))

static unsigned long pending = MAX_PENDING;

int lkl_cpu_get(void)
{
	if (__sync_fetch_and_add(&pending, 1) >= MAX_PENDING)
		return -1;

	lkl_ops->sem_down(cpu_lock);

	if (__sync_fetch_and_sub(&pending, 1) >= MAX_PENDING)
		return -1;

	return 0;
}

int lkl_cpu_try_get(void)
{
	int ret;

	if (__sync_fetch_and_add(&pending, 1) >= MAX_PENDING)
		return -1;

	ret = lkl_ops->sem_try_down(cpu_lock);

	if (__sync_fetch_and_sub(&pending, 1) >= MAX_PENDING)
		return -1;

	return ret;
}

void lkl_cpu_put(void)
{
	lkl_ops->sem_up(cpu_lock);
}

static struct lkl_sem *shutdown_sem;

void lkl_cpu_shutdown(void)
{
	__sync_fetch_and_add(&pending, MAX_PENDING);
}

void lkl_cpu_wait_shutdown(void)
{
	lkl_ops->sem_down(shutdown_sem);
	lkl_ops->sem_free(shutdown_sem);
}

static struct lkl_sem *idle_sem;

void arch_cpu_idle(void)
{
	if (pending >= MAX_PENDING) {

		while (__sync_fetch_and_add(&pending, 0) > MAX_PENDING)
			lkl_cpu_put();

		threads_cleanup();

		/* Shutdown the clockevents source. */
		tick_suspend_local();

		lkl_ops->sem_free(idle_sem);
		lkl_ops->sem_up(shutdown_sem);
		lkl_ops->sem_up(cpu_lock);
		lkl_ops->sem_free(cpu_lock);

		lkl_ops->thread_exit();
	}

	/* enable irqs now to allow direct irqs to run */
	local_irq_enable();

	/* we may have run irqs and sofirqs, check if we need to reschedule */
	if (need_resched())
		return;

	lkl_cpu_put();

	lkl_ops->sem_down(idle_sem);

	lkl_cpu_get();

	/* since we've been waked up its highly likely we have pending irqs */
	local_irq_disable();
	local_irq_enable();
}

void lkl_cpu_wakeup(void)
{
	lkl_ops->sem_up(idle_sem);
}

int lkl_cpu_init(void)
{
	cpu_lock = lkl_ops->sem_alloc(1);
	if (!cpu_lock)
		return -ENOMEM;

	idle_sem = lkl_ops->sem_alloc(0);
	if (!idle_sem) {
		lkl_ops->sem_free(cpu_lock);
		return -ENOMEM;
	}

	shutdown_sem = lkl_ops->sem_alloc(0);
	if (!shutdown_sem) {
		lkl_ops->sem_free(idle_sem);
		lkl_ops->sem_free(cpu_lock);
		return -ENOMEM;
	}

	pending = 0;

	return 0;
}
