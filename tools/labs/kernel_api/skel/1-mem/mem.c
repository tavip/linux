/*
 * SO2 lab3 - task 1
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ctype.h>

MODULE_DESCRIPTION("Print memory");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

static char *mem;

static inline int isprintable(char c)
{
        return (c >= 32 && c <= 126);
}

static int mem_init(void)
{
	size_t i;

	mem = kmalloc(4096 * sizeof(*mem), GFP_KERNEL);
	if (mem == NULL)
		goto err_mem;

	printk(LOG_LEVEL "chars: ");
	for (i = 0; i < 4096; i++) {
		if (isprintable(mem[i]))
			printk("%c ", mem[i]);
	}
	printk("\n");

	return 0;

err_mem:
	return -1;
}

static void mem_exit(void)
{
	kfree(mem);
}

module_init(mem_init);
module_exit(mem_exit);
