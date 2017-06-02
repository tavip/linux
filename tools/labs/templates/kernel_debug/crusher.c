#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Crusher - debugging module");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

int test;
module_param(test, int, 0);

typedef void (*foo_t)(void);

spinlock_t lock;
struct timer_list timer;

struct crush_struct {
	foo_t foo;
	int *p;
	char buf[4096];
};

struct crush_struct *g;

void crusher_foo(void)
{
	pr_info("Hello from foo function\n");
}

int crusher_simple_call(void)
{
	pr_info("crusher: simple call\n");
	crusher_foo();

	return 0;
}

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

void crusher_timer(unsigned long data)
{
	spin_lock(&lock);
	pr_info("crusher from timer: %s %d g: %p\n", current->comm, current->pid, g);
	spin_unlock(&lock);
}

int crusher_sync(void) {

	spin_lock_init(&lock);

	setup_timer(&timer, crusher_timer, 0);
	mod_timer(&timer, jiffies + 4 * HZ);

	spin_lock(&lock);
	pr_info("crusher from proces: %s %d, g %p\n", current->comm, current->pid, g);
	spin_unlock(&lock);

	return 0;
}

static int crusher_init(void)
{
	pr_info("crusher module loaded!\n");

	switch(test) {
	case 0:
		crusher_simple_call();
		break;
	case 1:
		crusher_invalid_access();
		break;
	case 2:
		crusher_use_before_init();
		break;
	case 3:
		crusher_use_after_free();
		break;
	case 4:
		crusher_alloc();
		crusher_free();
		break;
	case 5: 
		crusher_sync();
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
