/*
 * SO lab-02 - task 6 - cmd_mod.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Command-line args module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_WARNING

static char *str = "the worm";

module_param(str, charp, 0000);
MODULE_PARM_DESC(str, "A simple string");

static int __init cmd_init(void)
{
	printk(LOG_LEVEL "Early bird gets %s\n", str);
	return 0;
}

static void __exit cmd_exit(void)
{
	printk(LOG_LEVEL "Exit, stage left\n");
}

module_init(cmd_init);
module_exit(cmd_exit);
