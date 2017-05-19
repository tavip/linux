/*
 * SO2 lab3 - task 3
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 2: Allocate and initialize ti. */

	return ti;
}

static int memory_init(void)
{
	ti1 = task_info_alloc(/* TODO 1: current PID */);
	ti2 = task_info_alloc(/* TODO 1: parent PID */);
	ti3 = task_info_alloc(/* TODO 1: next process PID */);
	ti4 = task_info_alloc(/* TODO 1: next process of next process PID */);

	return 0;
}

static void memory_exit(void)
{
	/* TODO 3: Print ti* field values. */

	/* TODO 4: Free ti*. */
}

module_init(memory_init);
module_exit(memory_exit);
