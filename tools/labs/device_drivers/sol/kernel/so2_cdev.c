/*
 * SO2 Lab - Linux device drivers (#4)
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_DEBUG

#define MY_MAJOR		42
#define MY_MINOR		0
#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"hello\n"
#define IOCTL_MESSAGE		"Hello ioctl"

#ifndef BUFSIZ
#define BUFSIZ		4096
#endif


struct so2_device_data {
	struct cdev cdev;
	char buffer[BUFSIZ];
	size_t size;
	wait_queue_head_t wq;
	int flag;
	atomic_t access;
};

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data =
		container_of(inode->i_cdev, struct so2_device_data, cdev);

	file->private_data = data;

#ifndef EXTRA
	if (atomic_cmpxchg(&data->access, 1, 0) != 1)
		return -EBUSY;
#endif

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10);

	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	atomic_inc(&data->access);
#endif
	return 0;
}

static ssize_t
so2_cdev_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;

#ifdef EXTRA
	if (!data->size) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(data->wq, data->size != 0))
			return -ERESTARTSYS;
	}
#endif

	to_read = (size > data->size - *offset) ? (data->size - *offset) : size;
	if (copy_to_user(user_buffer, data->buffer + *offset, to_read) != 0)
		return -EFAULT;
	*offset += to_read;

	return to_read;
}

static ssize_t
so2_cdev_write(struct file *file,
		const char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;

	size = (*offset + size > BUFSIZ) ? (BUFSIZ - *offset) : size;

	if (copy_from_user(data->buffer + *offset, user_buffer, size) != 0)
		return -EFAULT;
	*offset += size;
	data->size = *offset;
#ifdef EXTRA
	wake_up_interruptible(&data->wq);
#endif

	return size;
}

static long
so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	int ret = 0;
	int remains;

	switch (cmd) {
	case MY_IOCTL_PRINT:
		printk(LOG_LEVEL "%s\n", IOCTL_MESSAGE);
		break;
	case MY_IOCTL_DOWN:
		data->flag = 0;
		ret = wait_event_interruptible(data->wq, data->flag != 0);
		break;
	case MY_IOCTL_UP:
		data->flag = 1;
		wake_up_interruptible(&data->wq);
		break;
	case MY_IOCTL_SET_BUFFER:
		remains = copy_from_user(data->buffer, (char __user *)arg,
				BUFFER_SIZE);
		if (remains)
			ret = -EFAULT;
		data->size = BUFFER_SIZE - remains;
		break;
	case MY_IOCTL_GET_BUFFER:
		if (copy_to_user((char __user *)arg, data->buffer, data->size))
			ret = -EFAULT;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
	.open = so2_cdev_open,
	.release = so2_cdev_release,
	.read = so2_cdev_read,
	.write = so2_cdev_write,
	.unlocked_ioctl = so2_cdev_ioctl,
};

static int so2_cdev_init(void)
{
	int err;
	int i;

	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR),
			NUM_MINORS, MODULE_NAME);
	if (err != 0) {
		printk(KERN_ERR "register_chrdev_region");
		return err;
	}

	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		devs[i].size = 0;
		memset(devs[i].buffer, 0, sizeof(devs[i].buffer));
#else
		devs[i].size = sizeof(MESSAGE);
		memcpy(devs[i].buffer, MESSAGE, sizeof(MESSAGE));
#endif
		init_waitqueue_head(&devs[i].wq);
		devs[i].flag = 0;
		atomic_set(&devs[i].access, 1);
		cdev_init(&devs[i].cdev, &so2_fops);
		cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++)
		cdev_del(&devs[i].cdev);

	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
