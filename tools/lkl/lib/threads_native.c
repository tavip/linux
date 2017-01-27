#include "lkl_host.h"
#include "threads.h"

struct lkl_thread {
	struct lkl_sem *sem;
	lkl_thread_t tid;
	void *info;
	char dead;
};

struct tba {
	struct lkl_thread *t;
	void (*fn)(void *arg);
	void *arg;
};

static void thread_bootstrap(void *arg)
{
	struct tba *tba = arg;

	lkl_host_ops.sem_down(tba->t->sem);
	tba->fn(tba->arg);
}

struct lkl_thread *thread_alloc(void (*fn)(void *), void *arg)
{
	struct lkl_thread *t;
	struct tba *tba;

	t = lkl_host_ops.mem_alloc(sizeof(*t));
	if (!t)
		return NULL;

	t->dead = 0;

	tba = lkl_host_ops.mem_alloc(sizeof(*tba));
	if (!tba) {
		lkl_host_ops.mem_free(t);
		return NULL;
	}

	tba->fn = fn;
	tba->arg = arg;
	tba->t = t;

	t->sem = lkl_host_ops.sem_alloc(0);
	if (!t->sem) {
		lkl_host_ops.mem_free(t);
		lkl_host_ops.mem_free(tba);
		return NULL;
	}

	t->tid = thread_create(thread_bootstrap, tba);
	if (!t->tid) {
		lkl_host_ops.mem_free(t);
		lkl_host_ops.mem_free(tba);
		lkl_host_ops.sem_free(t->sem);
		return NULL;
	}

	return t;
}


void thread_switch(struct lkl_thread *prev, struct lkl_thread *next)
{
	lkl_host_ops.sem_up(next->sem);
	if (!prev)
		return;

	lkl_host_ops.sem_down(prev->sem);

	if (prev->dead)
		thread_exit();
}


void thread_free(struct lkl_thread *thread)
{
	thread->dead = 1;
	lkl_host_ops.sem_up(thread->sem);
	if (!thread_equal(thread_self(), thread->tid))
	    thread_join(thread->tid);
	lkl_host_ops.sem_free(thread->sem);
	lkl_host_ops.mem_free(thread);
	if (thread_equal(thread_self(), thread->tid))
	    thread_exit();
}

static struct lkl_sem *idle_sem;

void enter_idle(void)
{
	if (!idle_sem) {
		idle_sem = lkl_host_ops.sem_alloc(0);
		if (!idle_sem)
			return;
	}

	lkl_host_ops.sem_down(idle_sem);
}

void exit_idle(void)
{
	if (!idle_sem) {
		idle_sem = lkl_host_ops.sem_alloc(0);
		if (!idle_sem)
			return;
	}

	lkl_host_ops.sem_up(idle_sem);
}
