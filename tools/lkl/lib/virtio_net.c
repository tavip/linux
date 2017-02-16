#include <string.h>
#include <lkl_host.h>
#include "virtio.h"
#include "endian.h"
#include "threads.h"
#include <lkl/linux/virtio_net.h>

#define netdev_of(x) (container_of(x, struct virtio_net_dev, dev))
#define BIT(x) (1ULL << x)

/* We always have 2 queues on a netdev: one for tx, one for rx. */
#define RX_QUEUE_IDX 0
#define TX_QUEUE_IDX 1

#define NUM_QUEUES (TX_QUEUE_IDX + 1)
#define QUEUE_DEPTH 128

/* In fact, we'll hit the limit on the devs string below long before
 * we hit this, but it's good enough for now. */
#define MAX_NET_DEVS 16

#ifdef DEBUG
#define bad_request(s) do {			\
		lkl_printf("%s\n", s);		\
		panic();			\
	} while (0)
#else
#define bad_request(s) lkl_printf("virtio_net: %s\n", s);
#endif /* DEBUG */

struct virtio_net_dev {
	struct virtio_dev dev;
	struct lkl_virtio_net_config config;
	struct lkl_netdev *nd;
};

static int net_check_features(struct virtio_dev *dev)
{
	if (dev->driver_features == dev->device_features)
		return 0;

	return -LKL_EINVAL;
}


/*
 * The buffers passed through "req" from the virtio_net driver always starts
 * with a vnet_hdr. We need to check the backend device if it expects vnet_hdr
 * and adjust buffer offset accordingly.
 */
static int net_enqueue(struct virtio_dev *dev, int q, struct virtio_req *req)
{
	struct lkl_virtio_net_hdr_v1 *header;
	struct virtio_net_dev *net_dev;
	struct iovec *iov;
	int ret;

	header = req->buf[0].iov_base;
	net_dev = netdev_of(dev);
	/*
	 * The backend device does not expect a vnet_hdr so adjust buf
	 * accordingly. (We make adjustment to req->buf so it can be used
	 * directly for the tx/rx call but remember to undo the change after the
	 * call.  Note that it's ok to pass iov with entry's len==0.  The caller
	 * will skip to the next entry correctly.
	 */
	if (!net_dev->nd->has_vnet_hdr) {
		req->buf[0].iov_base += sizeof(*header);
		req->buf[0].iov_len -= sizeof(*header);
	}
	iov = req->buf;

	/* Pick which virtqueue to send the buffer(s) to */
	if (q == TX_QUEUE_IDX) {
		ret = net_dev->nd->ops->tx(net_dev->nd, iov, req->buf_count);
		if (ret < 0)
			return -1;
	} else if (q == RX_QUEUE_IDX) {
		int i, len;

		ret = net_dev->nd->ops->rx(net_dev->nd, iov, req->buf_count);
		if (ret < 0)
			return -1;
		if (net_dev->nd->has_vnet_hdr) {
			/*
			 * If the number of bytes returned exactly matches the
			 * total space in the iov then there is a good chance we
			 * did not supply a large enough buffer for the whole
			 * pkt, i.e., pkt has been truncated.  This is only
			 * likely to happen under mergeable RX buffer mode.
			 */
			if (req->total_len == (unsigned int)ret)
				lkl_printf("PKT is likely truncated! len=%d\n",
				    ret);
		} else {
			header->flags = 0;
			header->gso_type = LKL_VIRTIO_NET_HDR_GSO_NONE;
		}
		/*
		 * Have to compute how many descriptors we've consumed (really
		 * only matters to the the mergeable RX mode) and return it
		 * through "num_buffers".
		 */
		for (i = 0, len = ret; len > 0; i++)
			len -= req->buf[i].iov_len;
		header->num_buffers = i;

		if (dev->device_features & BIT(LKL_VIRTIO_NET_F_GUEST_CSUM))
			header->flags = LKL_VIRTIO_NET_HDR_F_DATA_VALID;
	} else {
		bad_request("tried to push on non-existent queue");
		return -1;
	}
	if (!net_dev->nd->has_vnet_hdr) {
		/* Undo the adjustment */
		req->buf[0].iov_base -= sizeof(*header);
		req->buf[0].iov_len += sizeof(*header);
		ret += sizeof(struct lkl_virtio_net_hdr_v1);
	}
	virtio_req_complete(req, ret);
	return 0;
}

static struct virtio_dev_ops net_ops = {
	.check_features = net_check_features,
	.enqueue = net_enqueue,
};

struct virtio_net_dev *registered_devs[MAX_NET_DEVS];
static int registered_dev_idx = 0;

static int dev_register(struct virtio_net_dev *dev)
{
	if (registered_dev_idx == MAX_NET_DEVS) {
		lkl_printf("Too many virtio_net devices!\n");
		/* This error code is a little bit of a lie */
		return -LKL_ENOMEM;
	} else {
		/* registered_dev_idx is incremented by the caller */
		registered_devs[registered_dev_idx] = dev;
		return 0;
	}
}

static void virtio_net_poll(struct lkl_poller *p, enum lkl_poll_events events)
{
	struct lkl_netdev *nd =	container_of(p, struct lkl_netdev, poller);

	if ((events & LKL_POLLER_IN) || (events & LKL_POLLER_ALWAYS)) {
		nd->poller.events &= ~LKL_POLL_IN;
		virtio_process_queue(nd->dev, 0);
	}

	if ((events & LKL_POLLER_OUT) || (events & LKL_POLLER_ALWAYS)) {
		nd->poller.events &= ~LKL_POLL_OUT;
		virtio_process_queue(nd->dev, 1);
	}
}

int lkl_netdev_add(struct lkl_netdev *nd, struct lkl_netdev_args* args)
{
	struct virtio_net_dev *dev;
	int ret = -LKL_ENOMEM;

	dev = lkl_host_ops.mem_alloc(sizeof(*dev));
	if (!dev)
		return -LKL_ENOMEM;

	memset(dev, 0, sizeof(*dev));

	dev->dev.device_id = LKL_VIRTIO_ID_NET;
	if (args) {
		if (args->mac) {
			dev->dev.device_features |= BIT(LKL_VIRTIO_NET_F_MAC);
			memcpy(dev->config.mac, args->mac, LKL_ETH_ALEN);
		}
		dev->dev.device_features |= args->offload;

	}
	dev->dev.config_data = &dev->config;
	dev->dev.config_len = sizeof(dev->config);
	dev->dev.ops = &net_ops;
	dev->nd = nd;
	nd->dev = &dev->dev;

	ret = virtio_dev_setup(&dev->dev, NUM_QUEUES, QUEUE_DEPTH);

	if (ret)
		goto out_free;

	/*
	 * We may receive upto 64KB TSO packet so collect as many descriptors as
	 * there are available up to 64KB in total len.
	 */
	if (dev->dev.device_features & BIT(LKL_VIRTIO_NET_F_MRG_RXBUF))
		virtio_set_queue_max_merge_len(&dev->dev, RX_QUEUE_IDX, 65536);

	nd->poller.poll = virtio_net_poll;
	ret = lkl_poller_add(&nd->poller);
	if (ret < 0)
		goto out_cleanup_dev;

	ret = dev_register(dev);
	if (ret < 0)
		goto out_del_poller;

	return registered_dev_idx++;

out_del_poller:
	lkl_poller_del(&nd->poller);

out_cleanup_dev:
	virtio_dev_cleanup(&dev->dev);

out_free:
	lkl_host_ops.mem_free(dev);

	return ret;
}

/* Return 0 for success, -1 for failure. */
void lkl_netdev_remove(int id)
{
	struct virtio_net_dev *dev;
	int ret;

	if (id >= registered_dev_idx) {
		lkl_printf("%s: invalid id: %d\n", __func__, id);
		return;
	}

	dev = registered_devs[id];

	ret = lkl_netdev_get_ifindex(id);
	if (ret < 0) {
		lkl_printf("%s: failed to get ifindex for id %d: %s\n",
			   __func__, id, lkl_strerror(ret));
		return;
	}

	ret = lkl_if_down(ret);
	if (ret < 0) {
		lkl_printf("%s: failed to put interface id %d down: %s\n",
			   __func__, id, lkl_strerror(ret));
		return;
	}

	lkl_poller_del(&dev->nd->poller);

	virtio_dev_cleanup(&dev->dev);

	lkl_host_ops.mem_free(dev);
}

void lkl_netdev_free(struct lkl_netdev *nd)
{
	nd->ops->free(nd);
}
