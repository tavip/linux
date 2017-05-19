/*
 * SO2 - Block device drivers lab (#7)
 * Linux - Exercise #1, #2, #3, #6 (RAM Disk)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/genhd.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/vmalloc.h>

MODULE_DESCRIPTION("Simple RAM Disk");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");


#define KERN_LOG_LEVEL		KERN_ALERT

#define MY_BLOCK_MAJOR		240
#define MY_BLKDEV_NAME		"mybdev"
#define MY_BLOCK_MINORS		1
#define NR_SECTORS		128

#define KERNEL_SECTOR_SIZE	512

/* use bios for read/write requests */
#define USE_BIO_TRANSFER	1


static struct my_block_dev {
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
	u8 *data;
	size_t size;
} g_dev;

static int my_block_open(struct block_device *bdev, fmode_t mode)
{
	return 0;
}

static void my_block_release(struct gendisk *gd, fmode_t mode)
{
}

static const struct block_device_operations my_block_ops = {
	.owner = THIS_MODULE,
	.open = my_block_open,
	.release = my_block_release
};

static void my_block_transfer(struct my_block_dev *dev, sector_t sector,
		unsigned long len, char *buffer, int dir)
{
	unsigned long offset = sector * KERNEL_SECTOR_SIZE;

	if ((offset + len) > dev->size) /* beyond end read/write */
		return;

	if (dir == 1)		/* write */
		memcpy(dev->data + offset, buffer, len);
	else
		memcpy(buffer, dev->data + offset, len);
}

/* to transfer data using bio structures use #if 1 */
#if USE_BIO_TRANSFER == 1
static void my_xfer_request(struct my_block_dev *dev, struct request *req)
{
	struct bio_vec bvec;
	struct req_iterator iter;

	printk(KERN_ALERT "begin segment walk\n");
	rq_for_each_segment(bvec, req, iter) {
		sector_t sector = iter.iter.bi_sector;
		unsigned long offset = bvec.bv_offset;
		size_t len = bvec.bv_len;
		int dir = bio_data_dir(iter.bio);
		char *buffer = kmap_atomic(bvec.bv_page);

		printk(KERN_LOG_LEVEL "%s: buf %8p offset %lu len %u dir %d\n", __func__, buffer, offset, len, dir);
		my_block_transfer(dev, sector, len, buffer + offset, dir);
		kunmap_atomic(buffer);
	}
	printk(KERN_ALERT "complete segment walk\n");
}
#endif

static void my_block_request(struct request_queue *q)
{
	struct request *rq;
	struct my_block_dev *dev = q->queuedata;

	printk(KERN_ALERT "start request walk\n");
	while (1) {
		rq = blk_fetch_request(q);
		if (rq == NULL)
			break;

		if (rq->cmd_type != REQ_TYPE_FS) {
			printk(KERN_NOTICE "Skip non-fs request\n");
			__blk_end_request_all(rq, -EIO);
			continue;
		}

#if USE_BIO_TRANSFER == 1
		my_xfer_request(dev, rq);
#else
		my_block_transfer(dev, blk_rq_pos(rq),
				  blk_rq_bytes(rq),
				  bio_data(rq->bio), rq_data_dir(rq));
#endif


		__blk_end_request_all(rq, 0);
	}
	printk(KERN_ALERT "complete request walk\n");
}

static int create_block_device(struct my_block_dev *dev)
{
	int err;

	dev->size = NR_SECTORS * KERNEL_SECTOR_SIZE;
	dev->data = vmalloc(dev->size);
	if (dev->data == NULL) {
		printk(KERN_ERR "vmalloc: out of memory\n");
		err = -ENOMEM;
		goto out_vmalloc;
	}

	/* initialize the I/O queue */
	spin_lock_init(&dev->lock);
	dev->queue = blk_init_queue(my_block_request, &dev->lock);
	if (dev->queue == NULL) {
		printk(KERN_ERR "blk_init_queue: out of memory\n");
		err = -ENOMEM;
		goto out_blk_init;
	}
	blk_queue_logical_block_size(dev->queue, KERNEL_SECTOR_SIZE);
	dev->queue->queuedata = dev;

	/* initialize the gendisk structure */
	dev->gd = alloc_disk(MY_BLOCK_MINORS);
	if (!dev->gd) {
		printk(KERN_ERR "alloc_disk: failure\n");
		err = -ENOMEM;
		goto out_alloc_disk;
	}

	dev->gd->major = MY_BLOCK_MAJOR;
	dev->gd->first_minor = 0;
	dev->gd->fops = &my_block_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_name, DISK_NAME_LEN, "myblock");
	set_capacity(dev->gd, NR_SECTORS);

	add_disk(dev->gd);

	return 0;

out_alloc_disk:
	blk_cleanup_queue(dev->queue);
out_blk_init:
	vfree(dev->data);
out_vmalloc:
	return err;
}

static int __init my_block_init(void)
{
	int err = 0;

	err = register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
	if (err < 0) {
		printk(KERN_ERR "register_blkdev: unable to register\n");
		return err;
	}

	err = create_block_device(&g_dev);
	if (err < 0)
		goto out;

	return 0;

out:
	unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
	return err;
}

static void delete_block_device(struct my_block_dev *dev)
{
	if (dev->gd) {
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	if (dev->queue)
		blk_cleanup_queue(dev->queue);
	if (dev->data)
		vfree(dev->data);
}

static void __exit my_block_exit(void)
{
	delete_block_device(&g_dev);
	unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
}

module_init(my_block_init);
module_exit(my_block_exit);
