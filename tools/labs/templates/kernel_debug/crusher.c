#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("Crusher - sample m");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

int test;
module_param(test, int, 0);

typedef void (*foo_t)(void);

void crusher_foo(void)
{
	pr_info("Hello from foo function\n");
}

struct crush_struct {
	foo_t foo;
	int *p;
	char buf[4096];
};

struct crush_struct *g;

int crusher_invalid_access(void)
{
	struct crush_struct *cs;

	pr_info("crusher: invalid access...\n");

	cs = kzalloc(sizeof(cs), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	*cs->p = 42;

	return 0;
}

int crusher_use_before_init(void)
{
	struct crush_struct *cs;

	pr_info("crusher: use before init\n");

	cs = kmalloc(sizeof(*cs), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	cs->foo();

	return 0;
}

int crusher_use_after_free(void)
{
	struct crush_struct *cs;

	pr_info("crusher: use after free\n");

	cs = kmalloc(sizeof(*cs), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	cs->foo = crusher_foo;
	kfree(cs);

	cs->foo();

	return 0;
}

static int crusher_alloc(void)
{
	struct crush_struct *cs;

	pr_info("crusher: alloc\n");
	cs = kmalloc(sizeof(*cs), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;
	cs->p = kmalloc(42, sizeof(GFP_KERNEL));
	if (!cs->p)
		return -ENOMEM;

	g = cs;
	return 0;
}

static void crusher_free(void)
{
	pr_info("crusher: free\n");
	kfree(g);
}

static int crusher_init(void)
{
	pr_info("crusher module loaded!\n");

	switch(test) {
	case 0:
		crusher_invalid_access();
		break;
	case 1:
		crusher_use_before_init();
		break;
	case 2:
		crusher_use_after_free();
		break;
	case 3:
		crusher_alloc();
		crusher_free();
		break;
	default:
		pr_info("Test %d, not implemented\n", test);
		break;
	}

	return 0;
}

static void crusher_exit(void)
{
	pr_info("crusher module unloaded\n");
}

module_init(crusher_init);
module_exit(crusher_exit);
