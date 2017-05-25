/*
 * SO2 - Lab 6 - Deferred Work
 *
 * Exercise #6: kernel thread
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/kthread.h>

MODULE_DESCRIPTION("Simple kernel thread");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL KERN_DEBUG

wait_queue_head_t wq_stop_thread;
atomic_t flag_stop_thread;
wait_queue_head_t wq_thread_terminated;
atomic_t flag_thread_terminated;


int my_thread_f(void *data)
{
	printk(LOG_LEVEL "[my_thread_f] Current process id is %d (%s)\n",current->pid, current->comm);
	wait_event_interruptible(wq_stop_thread, atomic_read(&flag_stop_thread) != 0);

	atomic_set(&flag_thread_terminated, 1);
	wake_up_interruptible(&wq_thread_terminated);

	do_exit(0);
}

static int __init kthread_init(void)
{
	printk(LOG_LEVEL "[kthread_init] Init module\n");
	init_waitqueue_head(&wq_stop_thread);
	atomic_set(&flag_stop_thread, 0);
	init_waitqueue_head(&wq_thread_terminated);
	atomic_set(&flag_thread_terminated, 0);
	kthread_run(my_thread_f, NULL, "%skthread%d", "my", 0);

	return 0;
}

static void __exit kthread_exit(void)
{
	atomic_set(&flag_stop_thread, 1);
	wake_up_interruptible(&wq_stop_thread);
	wait_event_interruptible(wq_thread_terminated, atomic_read(&flag_thread_terminated) != 0);
	printk(LOG_LEVEL "[kthread_exit] Exit module\n");
}

module_init(kthread_init);
module_exit(kthread_exit);
