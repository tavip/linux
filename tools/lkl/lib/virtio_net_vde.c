#ifdef CONFIG_AUTO_LKL_VIRTIO_NET_VDE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <lkl.h>
#include <lkl_host.h>

#include "virtio.h"

#include <libvdeplug.h>

struct lkl_netdev_vde {
	struct lkl_netdev dev;
	struct lkl_poller poller;
	VDECONN *conn;
};

int net_vde_tx(struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
	int ret;
	struct lkl_netdev_vde *nd_vde =
		container_of(nd, struct lkl_netdev_vde, dev);
	void *data = iov[0].iov_base;
	int len = (int)iov[0].iov_len;

	ret = vde_send(nd_vde->conn, data, len, 0);
	if (ret <= 0) {
		if (errno == EAGAIN) {
			nd_vde->poller.events |= LKL_POLLER_OUT;
			lkl_poller_update(&nd_vde->poller);
		}
		return -1;
	}
	return ret;
}

int net_vde_rx(struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
	int ret;
	struct lkl_netdev_vde *nd_vde =
		container_of(nd, struct lkl_netdev_vde, dev);
	void *data = iov[0].iov_base;
	int len = (int)iov[0].iov_len;

	ret = vde_recv(nd_vde->conn, data, len, 0);
	if (ret <= 0) {
		if (errno == EAGAIN) {
			nd_vde->poller.events |= LKL_POLLER_IN;
			lkl_poller_update(&nd_vde->poller);
		}
		return -1;
	}
	return ret;
}


void net_vde_free(struct lkl_netdev *nd)
{
	struct lkl_netdev_vde *nd_vde =
		container_of(nd, struct lkl_netdev_vde, dev);

	vde_close(nd_vde->conn);
	free(nd_vde);
}

struct lkl_dev_net_ops vde_net_ops = {
	.tx = net_vde_tx,
	.rx = net_vde_rx,
	.free = net_vde_free,
};

struct lkl_netdev *lkl_netdev_vde_create(char const *switch_path)
{
	struct lkl_netdev_vde *nd;
	struct vde_open_args open_args = {.port = 0, .group = 0, .mode = 0700 };
	char *switch_path_copy = 0;

	nd = malloc(sizeof(*nd));
	if (!nd) {
		fprintf(stderr, "Failed to allocate memory.\n");
		/* TODO: propagate the error state, maybe use errno? */
		return 0;
	}
	nd->dev.ops = &vde_net_ops;


	/* vde_open() allows the null pointer as path which means
	 * "VDE default path"
	 */
	if (switch_path != 0) {
		/* vde_open() takes a non-const char * which is a bug in their
		 * function declaration. Even though the implementation does not
		 * modify the string, we shouldn't just cast away the const.
		 */
		size_t switch_path_length = strlen(switch_path);

		switch_path_copy = calloc(switch_path_length + 1, sizeof(char));
		if (!switch_path_copy) {
			fprintf(stderr, "Failed to allocate memory.\n");
			/* TODO: propagate the error state, maybe use errno? */
			return 0;
		}
		strncpy(switch_path_copy, switch_path, switch_path_length);
	}
	nd->conn = vde_open(switch_path_copy, "lkl-virtio-net", &open_args);
	free(switch_path_copy);
	if (nd->conn == 0) {
		free(nd);
		fprintf(stderr, "Failed to connect to vde switch.\n");
		/* TODO: propagate the error state, maybe use errno? */
		return NULL;
	}

	nd->poller.fd = vde_datafd(nd->conn);
	nd->poller.events = 0;

	return &nd->dev;
}

#else /* CONFIG_AUTO_LKL_VIRTIO_NET_VDE */

struct lkl_netdev *lkl_netdev_vde_create(char const *switch_path)
{
	fprintf(stderr, "lkl: The host library was compiled without support for VDE networking. Please rebuild with VDE enabled.\n");
	return 0;
}

#endif /* CONFIG_AUTO_LKL_VIRTIO_NET_VDE */
