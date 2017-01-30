#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <poll.h>
#include <lkl_host.h>
#include "iomem.h"
#include "threads.h"

/* Let's see if the host has semaphore.h */
#include <unistd.h>

#ifdef _POSIX_SEMAPHORES
#include <semaphore.h>
/* TODO(pscollins): We don't support fork() for now, but maybe one day
 * we will? */
#define SHARE_SEM 0
#endif /* _POSIX_SEMAPHORES */

static void print(const char *str, int len)
{
	int ret __attribute__((unused));

	ret = write(STDOUT_FILENO, str, len);
}

struct lkl_mutex {
	pthread_mutex_t mutex;
};

struct lkl_sem {
#ifdef _POSIX_SEMAPHORES
	sem_t sem;
#else
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
#endif /* _POSIX_SEMAPHORES */
};

struct lkl_tls_key {
	pthread_key_t key;
};

#define WARN_UNLESS(exp) do {						\
		if (exp < 0)						\
			lkl_printf("%s: %s\n", #exp, strerror(errno));	\
	} while (0)

static int _warn_pthread(int ret, char *str_exp)
{
	if (ret > 0)
		lkl_printf("%s: %s\n", str_exp, strerror(ret));

	return ret;
}


/* pthread_* functions use the reverse convention */
#define WARN_PTHREAD(exp) _warn_pthread(exp, #exp)

static struct lkl_sem *sem_alloc(int count)
{
	struct lkl_sem *sem;

	sem = malloc(sizeof(*sem));
	if (!sem)
		return NULL;

#ifdef _POSIX_SEMAPHORES
	if (sem_init(&sem->sem, SHARE_SEM, count) < 0) {
		lkl_printf("sem_init: %s\n", strerror(errno));
		free(sem);
		return NULL;
	}
#else
	pthread_mutex_init(&sem->lock, NULL);
	sem->count = count;
	WARN_PTHREAD(pthread_cond_init(&sem->cond, NULL));
#endif /* _POSIX_SEMAPHORES */

	return sem;
}

static void sem_free(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_destroy(&sem->sem));
#else
	WARN_PTHREAD(pthread_cond_destroy(&sem->cond));
	WARN_PTHREAD(pthread_mutex_destroy(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
	free(sem);
}

static void sem_up(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_post(&sem->sem));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	sem->count++;
	if (sem->count > 0)
		WARN_PTHREAD(pthread_cond_signal(&sem->cond));
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */

}

static void sem_down(struct lkl_sem *sem)
{
#ifdef _POSIX_SEMAPHORES
	int err;

	do {
		err = sem_wait(&sem->sem);
	} while (err < 0 && errno == EINTR);
	if (err < 0 && errno != EINTR)
		lkl_printf("sem_wait: %s\n", strerror(errno));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	while (sem->count <= 0)
		WARN_PTHREAD(pthread_cond_wait(&sem->cond, &sem->lock));
	sem->count--;
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
}

static struct lkl_mutex *mutex_alloc(void)
{
	struct lkl_mutex *_mutex = malloc(sizeof(struct lkl_mutex));
	pthread_mutex_t *mutex = NULL;
	pthread_mutexattr_t attr;

	if (!_mutex)
		return NULL;

	mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutexattr_init(&attr));

	/* PTHREAD_MUTEX_ERRORCHECK is *very* useful for debugging,
	 * but has some overhead, so we provide an option to turn it
	 * off. */
#ifdef DEBUG
	WARN_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
#endif /* DEBUG */

	WARN_PTHREAD(pthread_mutex_init(mutex, &attr));

	return _mutex;
}

static void mutex_lock(struct lkl_mutex *mutex)
{
	WARN_PTHREAD(pthread_mutex_lock(&mutex->mutex));
}

static void mutex_unlock(struct lkl_mutex *_mutex)
{
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_unlock(mutex));
}

static void mutex_free(struct lkl_mutex *_mutex)
{
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_destroy(mutex));
	free(_mutex);
}


lkl_thread_t thread_create(void (*fn)(void *), void *arg)
{
	pthread_t thread;
	if (WARN_PTHREAD(pthread_create(&thread, NULL, (void* (*)(void *))fn, arg)))
		return 0;
	else
		return (lkl_thread_t) thread;
}

void thread_detach(void)
{
	WARN_PTHREAD(pthread_detach(pthread_self()));
}

void thread_exit(void)
{
	pthread_exit(NULL);
}

int thread_join(lkl_thread_t tid)
{
	if (WARN_PTHREAD(pthread_join((pthread_t)tid, NULL)))
		return -1;
	else
		return 0;
}

lkl_thread_t thread_self(void)
{
	return (lkl_thread_t)pthread_self();
}

int thread_equal(lkl_thread_t a, lkl_thread_t b)
{
	return pthread_equal(a, b);
}

static struct lkl_tls_key *tls_alloc(void (*destructor)(void *))
{
	struct lkl_tls_key *ret = malloc(sizeof(struct lkl_tls_key));

	if (WARN_PTHREAD(pthread_key_create(&ret->key, destructor))) {
		free(ret);
		return NULL;
	}
	return ret;
}

static void tls_free(struct lkl_tls_key *key)
{
	WARN_PTHREAD(pthread_key_delete(key->key));
	free(key);
}

static int tls_set(struct lkl_tls_key *key, void *data)
{
	if (WARN_PTHREAD(pthread_setspecific(key->key, data)))
		return -1;
	return 0;
}

static void *tls_get(struct lkl_tls_key *key)
{
	return pthread_getspecific(key->key);
}

static unsigned long long time_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return 1e9*ts.tv_sec + ts.tv_nsec;
}

struct lkl_timer {
	timer_t timer;
};

static struct lkl_timer *timer_alloc(void (*fn)(void *), void *arg)
{
	int err;
	struct lkl_timer *timer;
	struct sigevent se =  {
		.sigev_notify = SIGEV_THREAD,
		.sigev_value = {
			.sival_ptr = arg,
		},
		.sigev_notify_function = (void (*)(union sigval))fn,
	};

	timer = malloc(sizeof(*timer));
	if (!timer)
		return NULL;

	err = timer_create(CLOCK_REALTIME, &se, &timer->timer);
	if (err) {
		free(timer);
		return NULL;
	}

	return timer;
}

static int timer_set_oneshot(struct lkl_timer *timer, unsigned long ns)
{
	struct itimerspec ts = {
		.it_value = {
			.tv_sec = ns / 1000000000,
			.tv_nsec = ns % 1000000000,
		},
	};

	return timer_settime(timer->timer, 0, &ts, NULL);
}

static void timer_free(struct lkl_timer *timer)
{
	timer_delete(&timer->timer);
	free(timer);
}

static void panic(void)
{
	assert(0);
}

static long _gettid(void)
{
#ifdef	__FreeBSD__
	return (long)pthread_self();
#else
	return syscall(SYS_gettid);
#endif
}

#define MAX_POLLERS 1024
static int max_poller = -1;
static struct pollfd pfds[MAX_POLLERS];
static struct lkl_poller *pollers[MAX_POLLERS];

int lkl_poller_add(struct lkl_poller *poller)
{
	int i;

	for(i = 0; i < MAX_POLLERS; i++) {
		if (!pollers[i]) {
			pollers[i] = poller;
			if (max_poller < i)
				max_poller = i;
			return 0;
		}
	}

	return -1;
}

int lkl_poller_del(struct lkl_poller *poller)
{
	int i;

	for(i = 0; i <= max_poller; i++) {
		if (pfds[i].fd == poller->fd) {
			pollers[i] = NULL;
			if (max_poller == i)
				max_poller--;
			return 0;
		}
	}

	return -1;
}

static struct lkl_poller poll_ctrl;
static int poll_ctrl_pipe[2];

static void poll_ctrl_kick(void)
{
	char tmp;
	int ret;

	ret = write(poll_ctrl_pipe[1], &tmp, sizeof(tmp));
	if (ret < 0)
		lkl_printf("%s: %s\n", __func__,  strerror(errno));
}

void lkl_poller_update(struct lkl_poller *poller)
{
	poll_ctrl_kick();
}


static void poll_ctrl_sink(struct lkl_poller *p, enum lkl_poll_events events)
{
	char tmp[PIPE_BUF];
	int ret;

	ret = read(poll_ctrl_pipe[0], tmp, sizeof(tmp));
	if (ret < 0 && errno != EAGAIN)
		lkl_printf("%s: %s\n", __func__, strerror(errno));
}

static int init(void)
{
	int ret;

	ret = pipe(poll_ctrl_pipe);
	if (ret)
		goto out;

	ret = fcntl(poll_ctrl_pipe[0], F_SETFL, O_NONBLOCK);
	if (ret)
		goto free_pipe;
	ret = fcntl(poll_ctrl_pipe[1], F_SETFL, O_NONBLOCK);
	if (ret)
		goto free_pipe;

	poll_ctrl.fd = poll_ctrl_pipe[0];
	poll_ctrl.events = LKL_POLLER_IN;
	poll_ctrl.poll = poll_ctrl_sink;

	return lkl_poller_add(&poll_ctrl);

free_pipe:
	close(poll_ctrl_pipe[0]);
	close(poll_ctrl_pipe[1]);
out:
	return ret;
}

static void posix_exit(void)
{
	close(poll_ctrl_pipe[0]);
	close(poll_ctrl_pipe[1]);
}

static void run_poll(int idle)
{
	int i, ret;
	int poll_timeout_ns = -1;

	if (!idle)
		poll_timeout_ns = 0;

	for(i = 0; i <= max_poller; i++) {
		if (!pollers[i]) {
			pfds[i].fd = -1;
			continue;
		}
		pfds[i].fd = pollers[i]->fd;
		pfds[i].events = 0;
		if (pollers[i]->events & LKL_POLLER_IN)
			pfds[i].events |= POLLIN|POLLPRI;
		if (pollers[i]->events & LKL_POLLER_OUT)
			pfds[i].events |= POLLOUT;
		if  (pollers[i]->timeout) {
			long timeout_ns = pollers[i]->timeout(pollers[i]);

			if (timeout_ns > 0 && (timeout_ns < poll_timeout_ns ||
					       poll_timeout_ns < 0))
				poll_timeout_ns = timeout_ns;
		}
	}

	ret = poll(pfds, max_poller + 1,
		   !poll_timeout_ns ? 0 : poll_timeout_ns / 10000000 + 1);
	if (ret < 0) {
		lkl_printf("%s: poll error: %s\n", __func__, strerror(errno));
		return;
	}

	for(i = 0; i <= max_poller; i++) {
		if (!pollers[i])
			continue;
		if (!pfds[i].revents &&
		    !(pollers[i]->events && LKL_POLLER_ALWAYS))
			continue;

		if (pollers[i]->poll) {
			enum lkl_poll_events events = 0;

			if (pfds[i].revents & (POLLIN|POLLPRI))
				events |= LKL_POLLER_IN;
			if (pfds[i].revents & POLLOUT)
				events |= LKL_POLLER_OUT;


			pollers[i]->poll(pollers[i], events);
		}
	}
}

static void posix_enter_idle(void)
{
	return run_poll(1);
}

static void posix_exit_idle(void)
{
	poll_ctrl_kick();
}

static void relax(void)
{
	return run_poll(0);
}

struct lkl_host_operations lkl_host_ops = {
	.init = init,
	.exit = posix_exit,
	.panic = panic,
	.thread_alloc = thread_alloc,
	.thread_switch = thread_switch,
	.thread_free = thread_free,
	.enter_idle = posix_enter_idle,
	.exit_idle = posix_exit_idle,
	.relax = relax,
	.sem_alloc = sem_alloc,
	.sem_free = sem_free,
	.sem_up = sem_up,
	.sem_down = sem_down,
	.mutex_alloc = mutex_alloc,
	.mutex_free = mutex_free,
	.mutex_lock = mutex_lock,
	.mutex_unlock = mutex_unlock,
	.tls_alloc = tls_alloc,
	.tls_free = tls_free,
	.tls_set = tls_set,
	.tls_get = tls_get,
	.time = time_ns,
	.timer_alloc = timer_alloc,
	.timer_set_oneshot = timer_set_oneshot,
	.timer_free = timer_free,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free,
	.ioremap = lkl_ioremap,
	.iomem_access = lkl_iomem_access,
	.virtio_devices = lkl_virtio_devs,
	.gettid = _gettid,
};

static int fd_get_capacity(struct lkl_disk disk, unsigned long long *res)
{
	off_t off;

	off = lseek(disk.fd, 0, SEEK_END);
	if (off < 0)
		return -1;

	*res = off;
	return 0;
}

static int do_rw(ssize_t (*fn)(), struct lkl_disk disk, struct lkl_blk_req *req)
{
	off_t off = req->sector * 512;
	void *addr;
	int len;
	int i;
	int ret = 0;

	for (i = 0; i < req->count; i++) {

		addr = req->buf[i].iov_base;
		len = req->buf[i].iov_len;

		do {
			ret = fn(disk.fd, addr, len, off);

			if (ret <= 0) {
				ret = -1;
				goto out;
			}

			addr += ret;
			len -= ret;
			off += ret;

		} while (len);
	}

out:
	return ret;
}

static int blk_request(struct lkl_disk disk, struct lkl_blk_req *req)
{
	int err = 0;

	switch (req->type) {
	case LKL_DEV_BLK_TYPE_READ:
		err = do_rw(pread, disk, req);
		break;
	case LKL_DEV_BLK_TYPE_WRITE:
		err = do_rw(pwrite, disk, req);
		break;
	case LKL_DEV_BLK_TYPE_FLUSH:
	case LKL_DEV_BLK_TYPE_FLUSH_OUT:
#ifdef __linux__
		err = fdatasync(disk.fd);
#else
		err = fsync(disk.fd);
#endif
		break;
	default:
		return LKL_DEV_BLK_STATUS_UNSUP;
	}

	if (err < 0)
		return LKL_DEV_BLK_STATUS_IOERR;

	return LKL_DEV_BLK_STATUS_OK;
}

struct lkl_dev_blk_ops lkl_dev_blk_ops = {
	.get_capacity = fd_get_capacity,
	.request = blk_request,
};

