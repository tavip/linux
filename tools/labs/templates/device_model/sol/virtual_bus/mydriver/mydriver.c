/*
 * SO2 - PnP Lab (#12)
 *
  */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
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
	char devname[NAME_SIZE];
	struct my_device mydev;				
	char buffer[PAGE_SIZE];
} dev_data;


/* driver */
static struct my_driver mydriver = {
	.module = THIS_MODULE,
	.driver = {
		.name = MODULE_NAME,
	},
};

// export a simple bus attribute
static ssize_t my_show_dev (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct my_device_data* my_data = (struct my_device_data *) dev_get_drvdata(dev);
	return snprintf (buf, PAGE_SIZE, "%d:%d\n", MAJOR(my_data->cdev.dev), MINOR(my_data->cdev.dev));
}
 
DEVICE_ATTR (myattr, 0444, my_show_dev , NULL);

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

static int match_name(struct device *dev, void *data)
{
        const char *name = data;
        return sysfs_streq(name, dev_name(dev));
}

static int my_init(void)
{
	int err;
	struct device *mdev;
	
	if((err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME)))
		return err;
	
	memset(dev_data.buffer, 0, PAGE_SIZE);
	sprintf(dev_data.devname, DEVICE_NAME);
	
	//register driver
	err = my_register_driver(&mydriver);
	if(err < 0) {
		printk(KERN_ERR "Unable to register driver\n");
		return err;
	}
	
	//register device
	dev_data.mydev.name = dev_data.devname;
	dev_data.mydev.driver = &mydriver;
	dev_set_drvdata(&dev_data.mydev.dev, &dev_data);
	err = my_register_device(&dev_data.mydev);
	if(err < 0) {
		printk(KERN_ERR "Unable to register device\n");
		return err;
	}
	
	//add attribute
	err = device_create_file (&dev_data.mydev.dev, &dev_attr_myattr);
	if(err)
		printk (KERN_ERR "Unable to create descr attribute\n");

	mdev = bus_find_device(mydriver.driver.bus,
                           NULL, "echo",
                           match_name);
	if (mdev != NULL) {
		printk(KERN_ERR "Found device %s\n", dev_name(mdev));
	}	

	cdev_init(&dev_data.cdev, &my_fops);
	cdev_add(&dev_data.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);
	return 0;
}

static void my_exit(void)
{
	device_remove_file (&dev_data.mydev.dev, &dev_attr_myattr);		
	//remove attribute
	my_unregister_driver(&mydriver);
	//unregister driver
	cdev_del(&dev_data.cdev);                                                   
	my_unregister_device(&dev_data.mydev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
}

module_init(my_init);
module_exit(my_exit);

