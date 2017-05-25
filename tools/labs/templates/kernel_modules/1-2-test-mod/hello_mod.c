/*
 * SO2 lab-02 - task 1 & 2 - hello_mod.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_hello_init(void)
{
	pr_info("Hello!\n");
	return 0;
}

static void hello_exit(void)
{
	pr_info("Goodbye!\n");
}

module_init(my_hello_init);
module_exit(hello_exit);
