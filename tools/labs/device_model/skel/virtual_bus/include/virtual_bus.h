/*
 * SO2 - PnP Lab (#12)
 *
 * definitions for the virtual bus/driver
 */

#ifndef _MY_VIRTUAL_BUS_H
#define _MY_VIRTUAL_BUS_H

#include <linux/device.h>
#include <linux/module.h>


/* Comment DEBUG macro to disable debugging. */
#define LOG_LEVEL   KERN_ALERT
#define DEBUG

#ifdef DEBUG
#define dprintk(fmt, args...) printk(LOG_LEVEL "[%s] " fmt, __FUNCTION__ , ##args)
#else
#define dprintk(fmt, args...)
#endif


extern struct bus_type my_bus_type;

struct my_device {
	char *name;
	struct my_driver *driver;
	struct device dev;
};

#define to_my_device(drv) container_of(dev, struct my_device, dev);

struct my_driver {
	struct module *module;
	struct device_driver driver;
};

#define to_my_driver(drv) container_of(drv, struct my_driver, driver);

int my_register_device (struct my_device *mydev);
void my_unregister_device (struct my_device * mydev);
int my_register_driver (struct my_driver *driver);
void my_unregister_driver (struct my_driver *driver);

#endif
