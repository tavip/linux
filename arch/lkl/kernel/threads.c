#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/host_ops.h>

static volatile int threads_counter;

unsigned long *alloc_thread_stack_node(struct task_struct *task, int node)
{
	struct thread_info *ti;

	ti = kmalloc(sizeof(*ti), GFP_KERNEL);
	if (!ti)
		return NULL;

	ti->task = task;
	return (unsigned long *)ti;
}

/*
 * The only new tasks created are kernel threads that have a predefined starting
 * point thus no stack copy is required.
 */
void setup_thread_stack(struct task_struct *p, struct task_struct *org)
{
	struct thread_info *ti = task_thread_info(p);
	struct thread_info *org_ti = task_thread_info(org);

	ti->flags = org_ti->flags;
	ti->preempt_count = org_ti->preempt_count;
	ti->addr_limit = org_ti->addr_limit;
}

static void kill_thread(struct thread_info *ti)
{
	lkl_ops->thread_free(ti->thread);
	__sync_fetch_and_sub(&threads_counter, 1);
}

void free_thread_stack(struct task_struct *tsk)
{
	struct thread_info *ti = task_thread_info(tsk);

	kill_thread(ti);
	kfree(ti);
}

struct thread_info *_current_thread_info = &init_thread_union.thread_info;

struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	struct thread_info *_prev = task_thread_info(prev);
	struct thread_info *_next = task_thread_info(next);
	/*
	 * schedule() expects the return of this function to be the task that we
	 * switched away from. Returning prev is not going to work because we
	 * are actually going to return the previous taks that was scheduled
	 * before the task we are going to wake up, and not the current task,
	 * e.g.:
	 *
	 * swapper -> init: saved prev on swapper stack is swapper
	 * init -> ksoftirqd0: saved prev on init stack is init
	 * ksoftirqd0 -> swapper: returned prev is swapper
	 */
	static struct task_struct *abs_prev = &init_task;

	abs_prev = prev;
	_current_thread_info = task_thread_info(next);
	lkl_ops->thread_switch(_prev->thread, _next->thread);
	return abs_prev;
}

struct thread_bootstrap_arg {
	struct thread_info *ti;
	int (*f)(void *);
	void *arg;
};

static void thread_bootstrap(void *_tba)
{
	struct thread_bootstrap_arg *tba = (struct thread_bootstrap_arg *)_tba;
	int (*f)(void *) = tba->f;
	void *arg = tba->arg;

	kfree(tba);
	f(arg);
	do_exit(0);
}

int copy_thread(unsigned long clone_flags, unsigned long esp,
		unsigned long unused, struct task_struct *p)
{
	struct thread_info *ti = task_thread_info(p);
	struct thread_bootstrap_arg *tba;

	tba = kmalloc(sizeof(*tba), GFP_KERNEL);
	if (!tba)
		return -ENOMEM;

	tba->f = (int (*)(void *))esp;
	tba->arg = (void *)unused;
	tba->ti = ti;

	ti->thread = lkl_ops->thread_alloc(thread_bootstrap, tba);
	if (!ti->thread) {
		kfree(tba);
		return -ENOMEM;
	}

	__sync_fetch_and_add(&threads_counter, 1);

	return 0;
}

void show_stack(struct task_struct *task, unsigned long *esp)
{
}

void threads_cleanup(void)
{
	struct task_struct *p;

	for_each_process(p) {
		struct thread_info *ti = task_thread_info(p);

		if (p->pid != 1)
			WARN(!(p->flags & PF_KTHREAD),
			     "non kernel thread task %p\n", p->comm);
		WARN(p->state == TASK_RUNNING,
		     "thread %s still running while halting\n", p->comm);

		kill_thread(ti);
	}

	while (threads_counter)
		;
}
