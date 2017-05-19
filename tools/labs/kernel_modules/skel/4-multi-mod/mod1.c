/*
 * SO2 lab-02 - task 4 - mod1.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Multi-file module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

extern int add(int a, int b);

static int n1, n2;

static int my_hello_init(void)
{
	n1 = 1; n2 = 2;
	printk(KERN_ALERT "n1 is %d, n2 is %d\n", n1, n2);

	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "sum id %d\n", add(n1, n2));
}

module_init(my_hello_init);
module_exit(hello_exit);
