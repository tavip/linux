/*
 * SO2 lab3 - task 5
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Full list processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

struct task_info {
	pid_t pid;
	unsigned long timestamp;
	atomic_t count;
	struct list_head list;
};

static struct list_head head;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 0: Copy from 3-memory and 4-list. */

	atomic_set(&ti->count, 0);

	return ti;
}

/*
 * Return pointer to struct task_info structure or NULL in case
 * pid argument isn't in the list.
 */

static struct task_info *task_info_find_pid(int pid)
{
	struct list_head *p;
	struct task_info *ti;

	/* TODO 1: Implement as in description. */
}

static void task_info_add_to_list(int pid)
{
	struct task_info *ti;

	ti = task_info_find_pid(pid);
	if (ti != NULL) {
		ti->timestamp = jiffies;
		atomic_inc(&ti->count);
		return;
	}

	/* TODO 0: Allocate item and then add to list. */
	/* Call task_info_alloc for allocation. */
	/* Copy from 4-memmory. */
}

static void task_info_add_for_current(void)
{
	/* TODO 0: Copy from 3-memory and 4-list. */
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

static void task_info_remove_expired(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		if (jiffies - ti->timestamp > 3 * HZ && atomic_read(&ti->count) < 5) {
			list_del(p);
			kfree(ti);
		}
	}
}

static void task_info_purge_list(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		list_del(p);
		kfree(ti);
	}
}

static int list_full_init(void)
{
	INIT_LIST_HEAD(&head);

	task_info_add_for_current();
	task_info_print_list("after first add");

	/* Wait 5 seconds so that all tasks become expired */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5 * HZ);

	return 0;
}

static void list_full_exit(void)
{
	struct task_info *ti;

	/* TODO 2: Ensure that at least one task is still available
	 * after calling task_info_remove_expired().
	 */
	/* ... */

	task_info_remove_expired();
	task_info_print_list("after removing expired");
	task_info_purge_list();
}

module_init(list_full_init);
module_exit(list_full_exit);
