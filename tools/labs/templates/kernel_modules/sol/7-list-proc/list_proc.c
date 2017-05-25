/*
 * SO2 lab-02 - task 7 - list_proc.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

static int my_proc_init(void)
{
	struct task_struct *p;

	printk(KERN_ALERT "Current process: pid = %d; comm = %s\n",
			current->pid, current->comm);

	printk(KERN_ALERT "\nProcess list:\n\n");
	for_each_process(p) {
		printk(KERN_ALERT "pid = %d; comm = %s\n",
				p->pid, p->comm);
	}

	return 0;
}

static void my_proc_exit(void)
{
	printk(KERN_ALERT "Current process: pid = %d; comm = %s\n",
			current->pid, current->comm);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

