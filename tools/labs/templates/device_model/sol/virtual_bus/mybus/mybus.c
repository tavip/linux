/*
 * PSO - PnP Lab (#12)
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include "../include/virtual_bus.h"

MODULE_AUTHOR ("PSO");
MODULE_LICENSE ("Dual BSD/GPL");
MODULE_DESCRIPTION ("MY virtual bus module");


/* BUS TYPE */

// match devices to drivers; just do a simple name test 
static int my_match (struct device *dev, struct device_driver *driver)
{
	return !strncmp (dev_name(dev), driver->name, strlen (driver->name));
}
 
// respond to hotplug user events
 static int my_uevent (struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEV_NAME=%s", dev_name(dev));
	return 0;
}

//bus type
struct bus_type my_bus_type = {
	.name	= "virtualbus",
	.match	= my_match,
	.uevent	= my_uevent,
};


/* BUS DEVICE */


// parent device release
static void my_bus_device_release (struct device *dev)
{
}

// parent device
static struct device my_bus_device = {
	.init_name   = "virtualbus0",
	.release  = my_bus_device_release
};

/* DEVICE (child) register / unregister */

/*
 * as we are not using the reference count, we use a no-op release function
 */
static void my_dev_release (struct device *dev)
{
}

int my_register_device (struct my_device *mydev)
{
	mydev->dev.bus = &my_bus_type;
	mydev->dev.parent = &my_bus_device;
	mydev->dev.release = my_dev_release;
	dev_set_name(&mydev->dev, mydev->name);
	//strncpy(mydev->dev.init_name, mydev->name, BUS_ID_SIZE);

	return device_register (&mydev->dev);
}

void my_unregister_device (struct my_device * mydev)
{
	device_unregister (&mydev->dev);
}

/* export register/unregister device functions */
EXPORT_SYMBOL (my_register_device);
EXPORT_SYMBOL (my_unregister_device);


/* DRIVER (child) register / unregister */

 
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
 
/* export register/unregister driver functions */
EXPORT_SYMBOL (my_register_driver);
EXPORT_SYMBOL (my_unregister_driver); 
 
 
/* INIT BUS (BUS TYPE and BUS DEVICE) */ 
 
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
