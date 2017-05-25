/* * SO2 lab3 - task 2
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Sleep while atomic");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

static int sched_spin_init(void)
{
	spinlock_t lock;
	spin_lock_init(&lock);

	//spin_lock(&lock);
	set_current_state(TASK_INTERRUPTIBLE);
	/* Try to sleep for 5 seconds. */
	schedule_timeout(5 * HZ);
	//spin_unlock(&lock);

	return 0;
}

static void sched_spin_exit(void)
{
}

module_init(sched_spin_init);
module_exit(sched_spin_exit);
