/*
 * POSIX file descriptor based virtual network interface feature for
 * LKL Copyright (c) 2015,2016 Ryo Nakamura, Hajime Tazaki
 *
 * Author: Ryo Nakamura <upa@wide.ad.jp>
 *         Hajime Tazaki <thehajime@gmail.com>
 *         Octavian Purdila <octavian.purdila@intel.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/uio.h>

#include "virtio.h"
#include "virtio_net_fd.h"

static int fd_net_tx(struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
	int ret;

	do {
		ret = writev(nd->poller.fd, iov, cnt);
	} while (ret == -1 && errno == EINTR);

	if (ret < 0) {
		if (errno != EAGAIN) {
			perror("write to fd netdev fails");
		} else {
			nd->poller.events |= LKL_POLLER_OUT;
			lkl_poller_update(&nd->poller);
		}
	}
	return ret;
}

static int fd_net_rx(struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
	int ret;

	do {
		ret = readv(nd->poller.fd, (struct iovec *)iov, cnt);
	} while (ret == -1 && errno == EINTR);

	if (ret < 0) {
		if (errno != EAGAIN) {
			perror("virtio net fd read");
		} else {
			nd->poller.events |= LKL_POLLER_IN;
			lkl_poller_update(&nd->poller);
		}
	}
	return ret;
}

static void fd_net_free(struct lkl_netdev *nd)
{
	close(nd->poller.fd);
	free(nd);
}

struct lkl_dev_net_ops fd_net_ops =  {
	.tx = fd_net_tx,
	.rx = fd_net_rx,
	.free = fd_net_free,
};

struct lkl_netdev *lkl_register_netdev_fd(int fd)
{
	struct lkl_netdev *nd;
	int ret;

	nd = malloc(sizeof(*nd));
	if (!nd) {
		fprintf(stderr, "fdnet: failed to allocate memory\n");
		/* TODO: propagate the error state, maybe use errno for that? */
		return NULL;
	}

	memset(nd, 0, sizeof(*nd));

	nd->poller.fd = fd;
	nd->poller.events = 0;
	nd->ops = &fd_net_ops;

	ret = lkl_poller_add(&nd->poller);
	if (ret) {
		free(nd);
		return NULL;
	}

	return nd;
}
