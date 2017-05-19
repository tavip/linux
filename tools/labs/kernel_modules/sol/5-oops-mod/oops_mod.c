/*
 * SO2 lab-02 - task 5 - oops_mod.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("Oops generating module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

static int my_oops_init(void)
{
	char *p = 0;

	printk(KERN_ALERT "before init\n");
	*p = 'a';
	printk(KERN_ALERT "after init\n");

	return 0;
}

static void my_oops_exit(void)
{
	printk(KERN_ALERT "module goes all out\n");
}

module_init(my_oops_init);
module_exit(my_oops_exit);
