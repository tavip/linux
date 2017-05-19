/*
 * SO2 lab3 - task 4
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Use list to process task info");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

struct task_info {
	pid_t pid;
	unsigned long timestamp;
	struct list_head list;
};

static struct list_head head;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 0: Copy from 3-memory. */

	return ti;
}

static void task_info_add_to_list(int pid)
{
	struct task_info *ti;

	/* TODO 1: Allocate item and then add to list. */
	/* Call task_info_alloc for allocation. */
}

static void task_info_add_for_current(void)
{
	/* TODO 0: Copy from 3-memory. */
	task_info_add_to_list(/* TODO */);
	task_info_add_to_list(/* TODO */);
	task_info_add_to_list(/* TODO */);
	task_info_add_to_list(/* TODO */);
}

static void task_info_print_list(const char *msg)
{
	struct list_head *p;
	struct task_info *ti;

	printk(LOG_LEVEL "%s: [ ", msg);
	list_for_each(p, &head) {
		ti = list_entry(p, struct task_info, list);
		printk("(%d, %lu) ", ti->pid, ti->timestamp);
	}
	printk("]\n");
}

static void task_info_purge_list(void)
{
	/* TODO 2: Remove all items from list. */
}

static int list_init(void)
{
	INIT_LIST_HEAD(&head);

	task_info_add_for_current();

	return 0;
}

static void list_exit(void)
{
	task_info_print_list("before exiting");
	task_info_purge_list();
}

module_init(list_init);
module_exit(list_exit);
