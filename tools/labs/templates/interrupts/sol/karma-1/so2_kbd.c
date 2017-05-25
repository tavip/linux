/*
 * SO2 Lab - Interrupts (#5)
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

MODULE_DESCRIPTION("SO2 KBD");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MODULE_NAME		"so2_kbd"

#define SO2_KBD_MAJOR		42
#define SO2_KBD_MINOR		0
#define SO2_KBD_NR_MINORS	1

#define I8042_KBD_IRQ		1
#define I8042_STATUS_REG	0x64
#define I8042_DATA_REG		0x60

#define BUFFER_SIZE		1024
#define SCANCODE_RELEASED_MASK	0x80

struct so2_device_data {
	struct cdev cdev;
	struct mutex mutex;
	spinlock_t lock;
	char buf[BUFFER_SIZE];
	size_t buf_idx;
	char tmp_buf[BUFFER_SIZE];
	size_t tmp_buf_idx;
} devs[1];

/*
 * Checks if scancode corresponds to key press or release.
 */
static int is_key_press(unsigned int scancode)
{
	return !(scancode & SCANCODE_RELEASED_MASK);
}

/*
 * Return the character of the given scancode.
 * Only works for alphanumeric/space/enter; returns '?' for other
 * characters.
 */
static int get_ascii(unsigned int scancode)
{
	static char *row1 = "1234567890";
	static char *row2 = "qwertyuiop";
	static char *row3 = "asdfghjkl";
	static char *row4 = "zxcvbnm";

	scancode &= ~SCANCODE_RELEASED_MASK;
	if (scancode >= 0x02 && scancode <= 0x0b)
		return *(row1 + scancode - 0x02);
	if (scancode >= 0x10 && scancode <= 0x19)
		return *(row2 + scancode - 0x10);
	if (scancode >= 0x1e && scancode <= 0x26)
		return *(row3 + scancode - 0x1e);
	if (scancode >= 0x2c && scancode <= 0x32)
		return *(row4 + scancode - 0x2c);
	if (scancode == 0x39)
		return ' ';
	if (scancode == 0x1c)
		return '\n';
	return '?';
}

/*
 * Return the value of the DATA register.
 */
static inline u8 i8042_read_data(void)
{
	u8 val;
	/* Read DATA register (8 bits). */
	val = inb(I8042_DATA_REG);
	return val;
}

/*
 * Handle IRQ 1 (keyboard IRQ).
 */
irqreturn_t so2_kbd_interrupt_handle(int irq_no, void *dev_id)
{
	unsigned int scancode;
	int pressed, ch;
	struct so2_device_data *data = (struct so2_device_data *) dev_id;

	scancode = i8042_read_data();
	pressed = is_key_press(scancode);
	ch = get_ascii(scancode);

	printk(LOG_LEVEL "IRQ %d: scancode=0x%x (%u) pressed=%d ch=%c\n",
		irq_no, scancode, scancode, pressed, ch);

	if (pressed) {
		spin_lock(&data->lock);
		if (data->buf_idx < BUFFER_SIZE) {
			/* Append character to buffer. */
			data->buf[data->buf_idx] = ch;
			data->buf_idx++;
		}
		spin_unlock(&data->lock);
	}

	return IRQ_NONE;
}

static int so2_kbd_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data =
		container_of(inode->i_cdev, struct so2_device_data, cdev);

	file->private_data = data;
	printk(LOG_LEVEL "%s opened\n", MODULE_NAME);
	return 0;
}

static int so2_kbd_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "%s closed\n", MODULE_NAME);
	return 0;
}

static ssize_t so2_kbd_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	unsigned long flags;
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;

	/* Readers are exclusive. */
	mutex_lock(&data->mutex);

	/*
	 * Copy information to temporary buffer.
	 * We need to disable local interrupts and we need locking
	 * to synchronize with interrupts on other CPUs.
	 */
	spin_lock_irqsave(&data->lock, flags);
	if (*offset > data->buf_idx) {
		spin_unlock_irqrestore(&data->lock, flags);
		mutex_unlock(&data->mutex);
		return 0;
	}
	data->tmp_buf_idx = data->buf_idx;
	memcpy(data->tmp_buf, data->buf, data->buf_idx);
	memset(data->buf, 0, data->buf_idx);
	data->buf_idx = 0;
	spin_unlock_irqrestore(&data->lock, flags);

	if (size > data->tmp_buf_idx - *offset)
		to_read = data->tmp_buf_idx - *offset;
	else
		to_read = size;
	if (copy_to_user(user_buffer, data->tmp_buf, to_read))
		return -EFAULT;

	/* Update offset. */
	*offset += to_read;

	mutex_unlock(&data->mutex);

	return to_read;
}

static const struct file_operations so2_kbd_fops = {
	.owner = THIS_MODULE,
	.open = so2_kbd_open,
	.release = so2_kbd_release,
	.read = so2_kbd_read,
};

static int so2_kbd_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS,
			MODULE_NAME);
	if (err != 0) {
		printk(LOG_LEVEL "ERROR: %s: error %d\n",
				"register_region", err);
		goto out;
	}

#if 0
	/* The following piece of code will fail, so leave it out. */
	if (request_region(I8042_DATA_REG, 1, MODULE_NAME) == NULL) {
		/* Will always fail, port is already used by keyboard driver. */
		/* cat /proc/iports | grep keyboard */
	}

	if (request_region(I8042_STATUS_REG, 1, MODULE_NAME) == NULL) {
		/* Will always fail, same as above. */
	}
#endif

	/* Initialize spinlock, mutex and buffers before requesting IRQ. */
	mutex_init(&devs[0].mutex);
	spin_lock_init(&devs[0].lock);

	memset(devs[0].buf, 0, BUFFER_SIZE);
	devs[0].buf_idx = 0;
	memset(devs[0].tmp_buf, 0, BUFFER_SIZE);
	devs[0].tmp_buf_idx = 0;

	/* Register IRQ handler for keyboard IRQ (IRQ 1). */
	err = request_irq(I8042_KBD_IRQ,
			so2_kbd_interrupt_handle,
			IRQF_SHARED, MODULE_NAME, &devs[0]);
	if (err != 0) {
		printk(LOG_LEVEL "ERROR: %s: error %d\n", "request_irq", err);
		goto out_unregister;
	}

	cdev_init(&devs[0].cdev, &so2_kbd_fops);
	cdev_add(&devs[0].cdev, MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR), 1);

	printk(LOG_LEVEL "Driver %s loaded\n", MODULE_NAME);
	return 0;

out_unregister:
	unregister_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS);
out:
	return err;
}

static void so2_kbd_exit(void)
{
	cdev_del(&devs[0].cdev);

	/* Free IRQ. */
	free_irq(I8042_KBD_IRQ, &devs[0]);

#if 0
	/* We didn't call request_region, so leave this out. */
	release_region(I8042_STATUS_REG, 1);
	release_region(I8042_DATA_REG, 1);
#endif

	unregister_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS);
	printk(LOG_LEVEL "Driver %s unloaded\n", MODULE_NAME);
}

module_init(so2_kbd_init);
module_exit(so2_kbd_exit);
