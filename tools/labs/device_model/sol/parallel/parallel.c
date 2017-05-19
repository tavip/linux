/*
 * SO2 lab PnP (#12)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pnp.h>

MODULE_DESCRIPTION("Parallel port module");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

//debug
#define LOG_LEVEL   KERN_ALERT
//comment this to remove debug messages
#define DEBUG

#ifdef DEBUG
#define dprintk(fmt, args...) printk(LOG_LEVEL "[%s] " fmt, __FUNCTION__ , ##args)
#else
#define dprintk(fmt, args...)
#endif


#define MY_BASEPORT 0x378
#define MY_NR_PORTS 8
#define MY_IRQ 7
#define MODULE_NAME "parallel"
#define MY_MAJOR	42 
#define MY_MINOR	0


//device data
struct my_device_data {
	struct cdev cdev;
	int irq;
	int iobase;
	int nr_ports;	
	struct device* my_classdev;
} mydev;

//doesn't support more than one device
static int count_dev = 0;

static const struct pnp_device_id parallel_pnp_dev_table[] = 
{
	/* Standard LPT Printer Port */
	{.id = "PNP0400", .driver_data = 0},
	{ }
};

MODULE_DEVICE_TABLE(pnp, parallel_pnp_dev_table);


static struct class my_class = {
	.name = "parclass",
};


static int parallel_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
	file->private_data = my_data;
	return 0;
}

static int parallel_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int parallel_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
	unsigned char ch;

	if(*offset > 0 || size < 1)
		return 0;

	ch = inb(MY_BASEPORT);
	dprintk("%x\n",ch);
	if(copy_to_user(user_buffer, &ch, 1))
		return -EFAULT;
	*offset = 1;	
	return 1;
}

static int parallel_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
	unsigned char ch;

	if(size < 1)
		return 0;
	if(copy_from_user(&ch, user_buffer, 1))
		return -EFAULT;
	outb(ch, MY_BASEPORT);
	dprintk("%x\n",ch);
	return 1;
}


struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = parallel_open,
	.read = parallel_read,
	.write = parallel_write,
	.release = parallel_release,
};


int register_parallel_dev(struct my_device_data * mydev, int baseport, int nr_ports) {

	if (!request_region(baseport, nr_ports, "parallelport")) {
		printk(KERN_ERR "Error requesting ports\n");
		return -ENODEV;
	}

	cdev_init(&mydev->cdev, &my_fops);
	cdev_add(&mydev->cdev, MKDEV(MY_MAJOR,MY_MINOR), 1);	
	return 0;
}

void unregister_parallel_dev(struct my_device_data * mydev, int baseport, int nr_ports) {
	release_region(baseport, nr_ports);
	cdev_del(&mydev->cdev);
}


static int parallel_pnp_probe(struct pnp_dev *dev, const struct pnp_device_id *dev_id)
{
	int err;	
	if(count_dev > 0)
		return -ENODEV;
	count_dev++;

	if (pnp_irq_valid(dev, 0))
		mydev.irq = pnp_irq(dev, 0);
	if (pnp_port_valid(dev, 0)) {
		mydev.iobase = pnp_port_start(dev, 0);
	} else
		return -ENODEV;
	mydev.nr_ports = pnp_port_len(dev, 0);

	err = register_parallel_dev(&mydev, mydev.iobase,mydev.nr_ports);
	if(err < 0) 
		return err;	

	mydev.my_classdev = device_create(&my_class, NULL, mydev.cdev.dev, &dev->dev, "parclass");	
	if(mydev.my_classdev == NULL) {
		printk(KERN_ERR "Unable to register class parclass");
		return -ENODEV;
	}

	dprintk("Registered parallel port: adress: %x-%x, irq: %d\n", mydev.iobase,  mydev.iobase + mydev.nr_ports, mydev.irq);		
	return 0;
}


static void parallel_pnp_remove(struct pnp_dev *dev)
{
	device_destroy(&my_class, mydev.cdev.dev);
	unregister_parallel_dev(&mydev, mydev.iobase,mydev.nr_ports);
	count_dev--;
}

static struct pnp_driver parallel_pnp_driver = {
	.name           = "parallel",
	.probe          = parallel_pnp_probe,
	.remove         = parallel_pnp_remove,
	.id_table       = parallel_pnp_dev_table,
};


static int parallel_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(MY_MAJOR,MY_MINOR), 1, MODULE_NAME);
	if(err < 0) {
		printk(KERN_ERR "Error register chardev region\n");
		return err;
	}	

	err = class_register(&my_class);
	if (err < 0) {
		printk(KERN_ERR "Error register class\n");
		return err;
	}

	err = pnp_register_driver (&parallel_pnp_driver);
	if(err < 0) {
		printk(KERN_ERR "Error register pnp driver\n");
		return err;
	}

	return 0;
}

static void parallel_exit(void)
{
	class_unregister(&my_class);
	pnp_unregister_driver (&parallel_pnp_driver);
	unregister_chrdev_region(MKDEV(MY_MAJOR,MY_MINOR), 1);
}

module_init(parallel_init);
module_exit(parallel_exit);

