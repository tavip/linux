/*
 * SO2 - Lab 6 - Deferred Work
 *
 * Exercise #1, #2: simple timer
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Simple kernel timer");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT
#define TIMER_TIMEOUT	1


static size_t nseconds;
static struct timer_list timer;

static void timer_handler(unsigned long var)
{
	nseconds += TIMER_TIMEOUT;
	printk(LOG_LEVEL "[timer_handler] nseconds = %d\n", nseconds);

	mod_timer(&timer, jiffies + TIMER_TIMEOUT * HZ);
}

static int __init timer_init(void)
{
	printk(LOG_LEVEL "[timer_init] Init module\n");

	nseconds = 0;
	setup_timer(&timer, timer_handler, 0);

	mod_timer(&timer, jiffies + TIMER_TIMEOUT * HZ);

	return 0;
}

static void __exit timer_exit(void)
{
	printk(LOG_LEVEL "[timer_exit] Exit module\n");

	del_timer_sync(&timer);
}

module_init(timer_init);
module_exit(timer_exit);
