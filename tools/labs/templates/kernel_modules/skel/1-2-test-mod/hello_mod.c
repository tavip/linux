/*
 * SO2 lab-02 - task 1 & 2 - hello_mod.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_DEBUG

static int my_hello_init(void)
{
	printk(LOG_LEVEL "Hello!\n");
	return 0;
}

static void hello_exit(void)
{
	printk(LOG_LEVEL "Goodbye!\n");
}

module_init(my_hello_init);
module_exit(hello_exit);
