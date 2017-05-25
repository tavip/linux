/*
 * SO2 - PnP Lab (#12)
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "../include/virtual_bus.h"

#define MY_MAJOR	42
#define MY_MINOR	0
#define MODULE_NAME	"echo"
#define DEVICE_NAME	"echo"
#define NAME_SIZE	20


MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct my_device_data {
	struct cdev cdev;
	char buffer[PAGE_SIZE];
	char devname[NAME_SIZE];
	/* TODO 2: Add field of type `struct my_device'.
	 * Initialization will be done in `my_init' function.
	 */
} dev_data;

/*
 * TODO 2: Add variable of type `struct my_driver'. Initialize module and
 * driver name fields inside the variable.
 */

/* TODO 3: Define a `my_show' function to be attached to a device attribute. */

/* TODO 3: Use DEVICE_ATTR() macro to define a `myattr' set of attributes. */

static int my_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
	file->private_data = my_data;
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
	struct my_device_data *my_data = (struct my_device_data*) file->private_data;
	size_t toberead, buffer_size;

	if(*offset > 0)
		return 0;
	buffer_size = strlen(my_data->buffer);
	toberead = (size < buffer_size) ? size : buffer_size;
	if(copy_to_user(user_buffer, my_data->buffer, toberead))
		return -EFAULT;
	*offset = toberead;
	return toberead;
}

static int my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
	struct my_device_data *my_data = (struct my_device_data*) file->private_data;
	size_t tobewritten;

	tobewritten = (size < PAGE_SIZE) ? size : PAGE_SIZE;
	memset(my_data->buffer, 0, PAGE_SIZE);
	if(copy_from_user(my_data->buffer, user_buffer, tobewritten))
		return -EFAULT;
	return tobewritten;
}


struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release,
};

static int my_init(void)
{
	int err;

	if((err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME)))
		return err;

	memset(dev_data.buffer, 0, PAGE_SIZE);
	sprintf(dev_data.devname, DEVICE_NAME);

	/*
	 * TODO 2: Register varabile of type `struct my_driver'.
	 * Use my_register() function in ../mybus/mybus.c.
	 */

	/*
	 * TODO 2: Initialize variable of type `struct my_device' (field of
	 * dev_data). Initialize name, driver.
	 * Initialize driver data to dev_data using dev_set_drvdata().
	 */

	/* TODO 2: Register variable of type `struct my_device'. */

	/* TODO 2.1: Check if the device is attached to the bus. If a device
	 * is returned by bus_find_device_by_name, then print it's name.
	 */
	

	/* TODO 3: Add `my_attr' attribute to device. */

	cdev_init(&dev_data.cdev, &my_fops);
	cdev_add(&dev_data.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);
	return 0;
}

static void my_exit(void)
{
	/* TODO 3: Remove attribute from device. */

	/* TODO 2: Unregister driver. */

	cdev_del(&dev_data.cdev);

	/* TODO 2: Unregister device. */

	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
}

module_init(my_init);
module_exit(my_exit);
