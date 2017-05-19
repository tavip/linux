/*
 * SO2 - Lab 6 - Deferred Work
 *
 * Exercises #3, #4, #5: deferred work
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "../include/deferred.h"

#define LOG_LEVEL 		KERN_ALERT
#define MY_MAJOR		42
#define MY_MINOR		0
#define MODULE_NAME		"deferred"

#define TIMER_TYPE_NONE		-1
#define TIMER_TYPE_SET		0
#define TIMER_TYPE_ALLOC	1
#define TIMER_TYPE_ACCT		2

MODULE_DESCRIPTION("Deferred work character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");


#define BUFFER_LEN	128

static struct my_device_data {
	struct cdev cdev;
	struct timer_list timer;
	int flag;
	struct work_struct work;
	spinlock_t lock;
	unsigned long buf[BUFFER_LEN];
	unsigned long tmp_buf[BUFFER_LEN];
	size_t tmp_buf_idx;
	size_t buf_idx;
} dev;

static void alloc_io(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5000);
	printk(LOG_LEVEL "Yawn! I've been sleeping for 5 seconds.\n");
}

static void work_handler(struct work_struct *work)
{
	alloc_io();
}

static void timer_handler(unsigned long var)
{
	struct my_device_data *my_data = (struct my_device_data *) var;

	switch (my_data->flag) {
	case TIMER_TYPE_SET:
		printk(LOG_LEVEL "[timer_handler] pid = %d, comm = %s\n", current->pid, current->comm);
		break;
	case TIMER_TYPE_ALLOC:
		schedule_work(&my_data->work);
		break;
	case TIMER_TYPE_ACCT:
		if (current->pid == 0) {
			spin_lock(&my_data->lock);
			if (my_data->buf_idx < BUFFER_LEN) {
				my_data->buf[my_data->buf_idx] = current->nivcsw;
				my_data->buf_idx++;
			}
			spin_unlock(&my_data->lock);
		}
		mod_timer(&my_data->timer, jiffies + HZ);
	default:
		break;
	}
}

static int deferred_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data =
		container_of(inode->i_cdev, struct my_device_data, cdev);
	file->private_data = my_data;
	printk(LOG_LEVEL "[deferred_open] Device opened\n");
	return 0;
}

static int deferred_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "[deferred_release] Device released\n");
	return 0;
}

static long deferred_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct my_device_data *my_data = (struct my_device_data*) file->private_data;

	printk(LOG_LEVEL "[deferred_ioctl] Command: %s\n", ioctl_command_to_string(cmd));

	switch (cmd) {
		case MY_IOCTL_TIMER_SET:
			my_data->flag = TIMER_TYPE_SET;
			mod_timer(&my_data->timer, jiffies + arg * HZ);
			break;
		case MY_IOCTL_TIMER_CANCEL:
			del_timer(&my_data->timer);
			break;
		case MY_IOCTL_TIMER_ALLOC:
			my_data->flag = TIMER_TYPE_ALLOC;
			mod_timer(&my_data->timer, jiffies + arg * HZ);
			break;
		case MY_IOCTL_TIMER_ACCT:
			my_data->flag = TIMER_TYPE_ACCT;
			mod_timer(&my_data->timer, jiffies + HZ);
			break;
		default:
			return -ENOTTY;
	}
	return 0;
}

static ssize_t deferred_read(struct file *file, char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct my_device_data *my_data =
		(struct my_device_data *) file->private_data;
	size_t to_read;

	spin_lock_bh(&my_data->lock);
	my_data->tmp_buf_idx = my_data->buf_idx;
	memcpy(my_data->tmp_buf, my_data->buf, my_data->tmp_buf_idx * sizeof(unsigned long));
	my_data->buf_idx = 0;
	spin_unlock_bh(&my_data->lock);

	if (size > my_data->tmp_buf_idx * sizeof(unsigned long) - *offset)
		to_read = my_data->tmp_buf_idx * sizeof(unsigned long) - *offset;
	else
		to_read = size;
	if (copy_to_user(user_buffer, my_data->tmp_buf + *offset, to_read) != 0)
		return -EFAULT;
	*offset += to_read;

	return to_read;
}

struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = deferred_open,
	.release = deferred_release,
	.unlocked_ioctl = deferred_ioctl,
	.read = deferred_read
};

static int deferred_init(void)
{
	int err;

	printk(LOG_LEVEL "[deferred_init] Init module\n");
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
	if (err) {
		printk(LOG_LEVEL "[deffered_init] register_chrdev_region: %d\n", err);
		return err;
	}

	dev.flag = TIMER_TYPE_NONE;
	INIT_WORK(&dev.work, work_handler);

	memset(dev.buf, 0, sizeof(dev.buf));
	memset(dev.tmp_buf, 0, sizeof(dev.tmp_buf));
	dev.buf_idx = 0;
	dev.tmp_buf_idx = 0;

	spin_lock_init(&dev.lock);

	cdev_init(&dev.cdev, &my_fops);
	cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);

	setup_timer(&dev.timer, timer_handler, (unsigned long) &dev);

	return 0;
}

static void deferred_exit(void)
{
	printk(LOG_LEVEL "[deferred_exit] Exit module\n" );

	cdev_del(&dev.cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);

	del_timer_sync(&dev.timer);

	flush_scheduled_work();
}

module_init(deferred_init);
module_exit(deferred_exit);
