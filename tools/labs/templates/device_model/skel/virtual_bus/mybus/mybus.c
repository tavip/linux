/*
 * SO2 - PnP Lab (#12)
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include "../include/virtual_bus.h"

MODULE_AUTHOR ("SO2");
MODULE_LICENSE ("Dual BSD/GPL");
MODULE_DESCRIPTION ("MY virtual bus module");


static int my_match (struct device *dev, struct device_driver *driver)
{
	return !strncmp (dev_name(dev), driver->name, strlen (driver->name));
}

static int my_uevent (struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEV_NAME=%s", dev_name(dev));
	return 0;
}

struct bus_type my_bus_type = {
	.name	= "mybus",
	.match	= my_match,
	.uevent	= my_uevent,
};


static void my_bus_device_release (struct device *dev)
{
}

/* Parent device structure. */
static struct device my_bus_device = {
	.init_name   = "mybus0",
	.release  = my_bus_device_release
};

static void my_dev_release (struct device *dev)
{
}

int my_register_device (struct my_device *mydev)
{
	mydev->dev.bus = &my_bus_type;
	mydev->dev.parent = &my_bus_device;
	mydev->dev.release = my_dev_release;
	dev_set_name(&mydev->dev, mydev->name);

	return device_register (&mydev->dev);
}

void my_unregister_device (struct my_device * mydev)
{
	device_unregister (&mydev->dev);
}

EXPORT_SYMBOL (my_register_device);
EXPORT_SYMBOL (my_unregister_device);



int my_register_driver (struct my_driver *driver)
{
	int ret;

	driver->driver.bus = &my_bus_type;
	ret = driver_register (&driver->driver);
	if (ret)
		return ret;
	return 0;
}

void my_unregister_driver (struct my_driver *driver)
{
	driver_unregister (&driver->driver);
}

EXPORT_SYMBOL (my_register_driver);
EXPORT_SYMBOL (my_unregister_driver);


static int __init my_bus_init (void)
{
	int ret;

	ret = bus_register (&my_bus_type);
	if (ret < 0) {
		printk(KERN_ERR "Unable to register bus\n");
		return ret;
	}

	ret = device_register(&my_bus_device);
	if (ret < 0)
		printk(KERN_NOTICE "Unable to register bus device\n");
	return 0;
}

static void my_bus_exit (void)
{
	device_unregister (&my_bus_device);
	bus_unregister (&my_bus_type);
}

module_init (my_bus_init);
module_exit (my_bus_exit);
