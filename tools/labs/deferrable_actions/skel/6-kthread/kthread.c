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
	printk(LOG_LEVEL "[my_thread_f] Current process id is %d (%s)\n", current->pid, current->comm);

	/* TODO: Wait for command to remove module on wq_stop_thread queue. */

	/*
	 * TODO: Before completing, notify completion using the
	 * wq_thread_terminated queue and its flag.
	 */

	do_exit(0);
}

static int __init kthread_init(void)
{
	printk(LOG_LEVEL "[kthread_init] Init module\n");

	/*
	 * TODO: Initialize the two wait queues and their flags:
	 *   1. wq_stop_thread: used for detecting the remove module
	 *   function being called
	 *   2. wq_thread_terminated: used for detecting the thread
	 *   completing its chore
	 */

	/* TODO: Start a kernel thread that executes my_thread_f(). */

	return 0;
}

static void __exit kthread_exit(void)
{
	/* TODO: Notify kernel thread waiting on wq_stop_thread queue. */

	/*
	 * TODO: Wait for kernel thread to complete on the
	 * wq_thread_terminated queue and its flag.
	 */

	printk(LOG_LEVEL "[kthread_exit] Exit module\n");
}

module_init(kthread_init);
module_exit(kthread_exit);
