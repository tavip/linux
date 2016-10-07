#include <lkl_host.h>
#include <ucontext.h>
#include <linux/list.h>

static LIST_HEAD(ready_list);

static void lkl_schedule(bool);

#define MAX_TLS_KEYS	10

struct lkl_thread {
	ucontext_t uc;
	struct list_head list;
	void *tls[MAX_TLS_KEYS];
	bool dead;
	bool detached;
	struct lkl_thread *join;
};

static struct lkl_thread *current;

static struct lkl_thread *lkl_thread_alloc(void)
{
	struct lkl_thread *t;

	t = lkl_host_ops.mem_alloc(sizeof(struct lkl_thread));
	if (!t)
		return NULL;

	t->dead = false;
	t->detached = false;
	memset(t->tls, 0, sizeof(t->tls));
	t->uc.uc_stack.ss_sp = NULL;
	t->uc.uc_link = NULL;

	return t;
}

static void lkl_thread_free(struct lkl_thread *t)
{
	if (t->uc.uc_stack.ss_sp)
		lkl_host_ops.mem_free(t->uc.uc_stack.ss_sp);
	lkl_host_ops.mem_free(t);
}

static void uc_thread_bootstrap();

static lkl_thread_t thread_create(void (*fn)(void *), void *arg)
{
	struct lkl_thread *t = lkl_thread_alloc();
	uint64_t _fn = (uint64_t)fn;
	uint64_t _arg = (uint64_t)arg;

//	lkl_printf("%s: %p %p\n", __func__, fn, arg);

	if (!current) {
		current = lkl_thread_alloc();
		if (!current) {
			lkl_printf("failed to create initial ucontext\n");
			lkl_host_ops.panic();
		}
	}

	if (!t)
		return 0;

	getcontext(&t->uc);
	t->uc.uc_stack.ss_size = 128*1024;
	t->uc.uc_stack.ss_sp = lkl_host_ops.mem_alloc(t->uc.uc_stack.ss_size);
	if (!t->uc.uc_stack.ss_sp) {
		lkl_thread_free(t);
		return 0;
	}

	makecontext(&t->uc, uc_thread_bootstrap, 4, (int)(_fn & 0xffffffff),
		    (int)(_fn >> 32), (int)(_arg & 0xffffffff),
		    (int)(_arg >> 32));

	list_add_tail(&t->list, &ready_list);


	return (lkl_thread_t)t;
}

static void uc_thread_bootstrap(int fn_low, int fn_high,
				int arg_low, int arg_high)
{
	uint64_t _fn = (uint32_t)fn_low | ((uint64_t)fn_high << 32);
	uint64_t _arg = (uint32_t)arg_low | ((uint64_t)arg_high << 32);
	void (*fn)(void *) = (void (*)(void *))_fn;
	void *arg = (void *)_arg;

//	lkl_printf("%s: %p %p %llx\n", __func__, fn, arg, _arg);

	fn(arg);
}

static int thread_join(lkl_thread_t tid)
{
	struct lkl_thread *t = (struct lkl_thread *)tid;

//	lkl_printf("%s: %p\n", __func__, t);

	if (t->detached)
		return -1;

	if (t->dead) {
		lkl_thread_free(t);
	} else {
//		lkl_printf("%s: wait\n", __func__, t);
		t->join = current;
		lkl_schedule(false);
	}

	return 0;
}

static lkl_thread_t thread_self(void)
{
	return (lkl_thread_t)current;
}

static int thread_equal(lkl_thread_t a, lkl_thread_t b)
{
	return a == b;
}


struct lkl_mutex {
	int lock;
	int recursive;
	lkl_thread_t owner;
	 /* list of threads waiting on this mutex */
	struct list_head list;
};

struct lkl_sem {
	int count;
	/* list of threads waiting on this sem */
	struct list_head list;
};

static struct lkl_mutex *mutex_alloc(int recursive)
{
	struct lkl_mutex *mutex;

	mutex = lkl_host_ops.mem_alloc(sizeof(struct lkl_mutex));
	if (!mutex)
		return NULL;

	mutex->lock = 0;
	mutex->recursive = 1;
	mutex->owner = 0;
	INIT_LIST_HEAD(&mutex->list);

	return mutex;
}

static void mutex_free(struct lkl_mutex *mutex)
{
	lkl_host_ops.mem_free(mutex);
}

static void mutex_lock(struct lkl_mutex *mutex)
{
	while (mutex->lock &&
	       (!mutex->recursive || !thread_equal(mutex->owner, thread_self()))) {
		list_add_tail(&current->list, &mutex->list);
		lkl_schedule(false);
	}

	mutex->owner = thread_self();
	mutex->lock++;
}

static void mutex_unlock(struct lkl_mutex *mutex)
{
	struct lkl_thread *t;

	if (--mutex->lock)
		return;

	if (list_empty(&mutex->list))
		return;

	t = list_first_entry(&mutex->list, struct lkl_thread, list);
	list_del(&t->list);
	list_add_tail(&t->list, &ready_list);
}

static struct lkl_sem *sem_alloc(int count)
{
	struct lkl_sem *sem = lkl_host_ops.mem_alloc(sizeof(struct lkl_sem));

	if (!sem)
		return NULL;

	sem->count = count;
	INIT_LIST_HEAD(&sem->list);

	return sem;
}

static void sem_free(struct lkl_sem *sem)
{
	lkl_host_ops.mem_free(sem);
}

static void sem_down(struct lkl_sem *sem)
{
	while (sem->count <= 0) {
		list_add(&current->list, &sem->list);
		lkl_schedule(false);
	}

	sem->count--;
}

static void sem_up(struct lkl_sem *sem)
{
	struct lkl_thread *t;

	if (++sem->count <= 0)
		return;

	if (list_empty(&sem->list))
		return;

	t = list_first_entry(&sem->list, struct lkl_thread, list);
	list_del(&t->list);
	list_add(&t->list, &ready_list);
}

static void thread_detach(void)
{
	current->detached = true;
}

static char tls_keys[MAX_TLS_KEYS];
typedef void (*tls_destructor)(void *);
static tls_destructor tls_destructors[MAX_TLS_KEYS];

static void thread_exit(void)
{
	int i;

	for(i = 0; i < MAX_TLS_KEYS; i++) {
		if (tls_keys[i] && tls_destructors[i])
			tls_destructors[i](current->tls[i]);
	}
	lkl_schedule(true);
}

static int tls_alloc(unsigned int *key, void (*destructor)(void *))
{
	int i;

	for(i = 0; i < MAX_TLS_KEYS; i++) {
		if (!tls_keys[i]) {
			tls_keys[i] = 1;
			*key = i;
			return 0;
		}
	}

	return -1;
}

static int tls_free(unsigned int key)
{
	if (key >= MAX_TLS_KEYS || !tls_keys[key])
		return -1;
	tls_keys[key] = 0;
	tls_destructors[key] = NULL;
	return 0;
}

static int tls_set(unsigned int key, void *data)
{
	if (key >= MAX_TLS_KEYS || !tls_keys[key])
		return -1;
	current->tls[key] = data;
	return 0;
}

static void *tls_get(unsigned int key)
{
	if (key >= MAX_TLS_KEYS || !tls_keys[key])
		return NULL;
	return current->tls[key];
}

unsigned long csws;

static __attribute((noinline)) void lkl_schedule(bool exit)
{
	struct lkl_thread *next, *prev;

//	lkl_printf("%s: %d\n", __func__, exit);

	if (exit && current->join)
		list_add_tail(&current->join->list, &ready_list);

	if (list_empty(&ready_list)) {
		if (exit)
			lkl_host_ops.panic();
		return;
	}

	next = list_first_entry(&ready_list, struct lkl_thread, list);
	list_del(&next->list);

	prev = current;
	current = next;

	csws++;

	if (exit) {
		if (prev->detached)
			lkl_thread_free(prev);
	        else
			prev->dead = true;
		setcontext(&next->uc);
	} else {
		swapcontext(&prev->uc, &next->uc);
	}
}

void thread_yield(void)
{
	list_add_tail(&current->list, &ready_list);
	lkl_schedule(false);
}
