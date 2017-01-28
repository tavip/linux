#ifndef _LKL_LIB_THREADS_H
#define _LKL_LIB_THREADS_H

struct lkl_thread;

struct lkl_thread *thread_alloc(void (*fn)(void *), void *arg);
void thread_switch(struct lkl_thread *prev, struct lkl_thread *next);
void thread_free(struct lkl_thread *thread);

#endif /* _LKL_LIB_THREADS_H */
