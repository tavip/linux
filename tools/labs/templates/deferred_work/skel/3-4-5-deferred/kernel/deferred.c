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
	/* TODO 1: Add timer here. */
	/* TODO 2: Add flag here; will be used with TIMER_TYPE_* values.  */
	/* TODO 3: Add work_struct here. */

	/* fields for karma task */
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

/* TODO 3: Define work handler. */

static void timer_handler(unsigned long var)
{
	/* TODO 1: Implement timer handler. */

	/* TODO 2: Take into account flag value (TIMER_TYPE_SET or TIMER_TYPE_ALLOC). */

	/* TODO 3: Schedule work for TIMER_TYPE_ALLOC. */

	/*
	 * TODO karma:
	 * Account involuntary context switches for swapper process.
	 * swapper/0 process has PID 0.
	 * Take into account flag value TIMER_TYPE_ACCT.
	 * Use spinlock to synchronize with read routine.
	 * Only add involuntary context switch number (nivcsw) if room in
	 * buffer (and PID is 0).
	 * Reschedule timer to run in another second.
	 */
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
			/* TODO 1: Set flag to TIMER_TYPE_SET. */
			/* TODO 1: Schedule timer to print the pid and comm fields. */
			break;
		case MY_IOCTL_TIMER_CANCEL:
			/* TODO 1: Cancel timer. */
			break;
		case MY_IOCTL_TIMER_ALLOC:
			/* TODO 2: Set flag to TIMER_TYPE_ALLOC. */
			/* TODO 2: Schedule timer to call alloc_io(). */
			break;
		case MY_IOCTL_TIMER_ACCT:
			/* TODO karma: Set flag to TIMER_TYPE_ACCT. */
			/* TODO karma: Schedule timer to run in one second. */
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

	/*
	 * TODO karma:
	 * Copy information in my_data->tmp_buf to copy it to user space.
	 * Reset my_data->buf.
	 * This needs to happen using locking and bottom halves disabled
	 * to prevent simultaneously tampering with my_data->buf and
	 * my_data->buf_idx.
	 */

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

	/* TODO 1: Initialize timer. */

	/* TODO 2: Initialize flag. */

	/* TODO 3: Initialize work. */

	/* TODO karma: Initialize lock and buffers. */

	cdev_init(&dev.cdev, &my_fops);
	cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);

	return 0;
}

static void deferred_exit(void)
{
	printk(LOG_LEVEL "[deferred_exit] Exit module\n" );

	cdev_del(&dev.cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);

	/* TODO 1: Cleanup: make sure the timer is not running after exiting. */

	/* TODO 3: Cleanup: make sure the work handler is not scheduled. */
}

module_init(deferred_init);
module_exit(deferred_exit);
