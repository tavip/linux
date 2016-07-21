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

void lkl_cpu_get(void)
{
	lkl_ops->sem_down(cpu_lock);
}

bool lkl_cpu_try_get(void)
{
	return lkl_ops->sem_try_down(cpu_lock);
}

void lkl_cpu_put(void)
{
	lkl_ops->sem_up(cpu_lock);
}

static bool shutdown;
static struct lkl_sem *shutdown_sem;

void lkl_cpu_shutdown(void)
{
	shutdown = true;
}

bool lkl_cpu_is_shutdown(void)
{
	return shutdown;
}

void lkl_cpu_wait_shutdown(void)
{
	lkl_ops->sem_down(shutdown_sem);
	lkl_ops->sem_free(shutdown_sem);
}

static struct lkl_sem *idle_sem;

void arch_cpu_idle(void)
{
	if (shutdown) {
		threads_cleanup();

		/* Shutdown the clockevents source. */
		tick_suspend_local();

		lkl_ops->sem_free(idle_sem);
		lkl_ops->sem_up(shutdown_sem);
		lkl_ops->sem_up(cpu_lock);
		lkl_ops->sem_free(cpu_lock);

		lkl_ops->thread_exit();
	}

	lkl_cpu_put();

	lkl_ops->sem_down(idle_sem);

	lkl_cpu_get();

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

	return 0;
}
