/*
 * SO2 lab-02 - task 3 - err_mod.c
 */

#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Error module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

static int n1, n2;

static int err_init(void)
{
	n1 = 1; n2 = 2;
	printk(KERN_ALERT "n1 is %d, n2 is %d\n", n1, n2);

	return 0;
}

static void err_exit(void)
{
	printk(KERN_ALERT "sum is %d\n", n1 + n2);
}

module_init(err_init);
module_exit(err_exit);
