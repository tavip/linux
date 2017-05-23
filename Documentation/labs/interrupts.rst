===================
I/O and Interrupts 
===================


Lab objectives
==============

* communication with peripheral devices
* implement interrupt handlers
* synchronizing interrupts with process context

Keywords: IRQ, I/O port, I/O address, base address, UART, request_region, release_region, inb, outb

Background information
======================


Hardware access
---------------

A peripheral device is controlled by writing and reading its
registers. Often, a device has multiple registers that can be accessed
at consecutive addresses either in the memory address space or in the
I/O address space. Each device connected to the I/O bus has a set of
I/O addresses, called I/O ports. I/O ports can be mapped to physical
memory addresses so that the processor can communicate with the device
through instructions that work directly with the memory. For
simplicity we will directly use I/O ports (without mapping to physical
memory addresses) to communicate with physical devices.


The I/O ports of each device are structured into a set of specialized
registers to provide a uniform programming interface. Thus, most
devices will have the following types of registers:

* **Control** registers that receive device commands
* **Status** registers, which contain information about the device's
  internal status
* **Input** registers from which data is taken from the device
* **Output** registers in which the data is written to transmit it to the
  device
    
Physical ports are differentiated by the number of bits: they can be
8, 16 or 32-bit ports.

For example, the parallel port has 8 8-bit I/O ports starting at base
address 0x378. The data log is found at base address (0x378), status
register at base + 1 (0x379), and control at base address + 2
(0x37a). The data log is both an entry and exit log.

Interrupt handling
------------------

Although there are devices that can be fully controlled using I/O
ports or special memory areas, there are situations where this is
insufficient. The main problem that needs to be addressed is that
certain events occur at undefined moments in time and it is
inefficient for the processor (CPU) to interrogate the status of the
device repeatedly (polling). The way to solve this problem is using an
Interrupt ReQuest (IRQ) which is a hardware notification by which the
processor is announced that a particular external event happened.

For IRQs to be useful device drivers must implement handlers, i.e. a
particular sequence of code that handles the interrupt. Because in
many situations the number of interrupts available is limited, a
device driver must behave in an orderly fashion with interruptions:
interrupts must be required before being used and released when they
are no longer needed. In addition, in some situations, device drivers
must share an interrupt or synchronize with interrupts. All of these will be
discussed further.

Locking
-------

When we need to access shared resources between an interrupt
routine (A) and code running in process context or in bottom-half
context (B), we must use a special synchronization technique. In (A)
we need to use a spinlock primitive, and in (B) we must disable
interrupts AND use a spinlock primitive. Disabling interrupts is not
enough because the interrupt routine can run on a processor other than
the one running (B).

Using only a spinlock can lead to a deadlock. The classic example of
deadlock in this case is:

1. We run a process process on the X processor, and we acquire the lock
2. Before releasing the lock, an interrupt is generated on the X processor
3. The interrupt handling routine will try to acquire the lock and it
   will go into an infinite loop


Accessing the hardware in Linux
===============================

In Linux, the I/O ports access is implemented on all architectures and
there are several APIs that can be used.

Request access to I/O ports 
---------------------------

Before accessing I/O ports we first must request access to them, to make sure there is only one user. In order to do so, one must use the :c:func:`request_region` function:

.. kernel-doc:: include/linux/ioport.h
   :functions: request_region

To release a reserved region one must use the :c:func:`release_region` function:

.. kernel-doc:: include/linux/ioport.h
   :functions: release_region


For example, the serial port COM1 has the base address 0x3F8 and it
has 8 ports and this is a code snippet of how to request access to
these ports:

.. code-block:: c

   #include <linux/ioport.h>

   #define MY_BASEPORT 0x3F8
   #define MY_NR_PORTS 8
  
   if (!request_region(MY_BASEPORT, MY_NR_PORTS, "com1")) {
   	/* handle error */
   	return -ENODEV;
   }

To release the ports one would use something like:

.. code-block:: c

   release_region(MY_BASEPORT, MY_NR_PORTS);

Most of the time, port requests are done at the driver initialization
or probe time and the port releasing is done at the removal of the
device or module.

All of the port requests can be seen from userspace via the /proc/ioports file:

.. code-block:: shell

   $ cat /proc/ioports
   0000-001f : dma1
   0020-0021 : pic1
   0040-005f : timer
   0060-006f : keyboard
   0070-0077 : rtc
   0080-008f : dma page reg
   00a0-00a1 : pic2
   00c0-00df : dma2
   00f0-00ff : fpu
   0170-0177 : ide1
   01f0-01f7 : ide0
   0376-0376 : ide1
   0378-037a : parport0
   037b-037f : parport0
   03c0-03df : vga+
   03f6-03f6 : ide0
   03f8-03ff : serial
   ...

Accessing I/O ports
-------------------

After a driver has obtained the desired I/O port range, one can
perform read or write operations on these ports. Since physical ports
are differentiated by the number of bits (8, 16, or 32 bits), there
are different port access functions depending on their size. The
following port access functions are defined in asm/io.h:


* *unsigned inb(int port)*, read one byte (8 bits) from port
* *void outb(unsigned char byte, int port)*, write one byte (8 bits) to port
* *unsigned inw(int port)*, read two bytes (16-bit) ports
* *void outw(unsigned short word, int port)* writes two bytes (16-bits) to port
* *unsigned inl (int port)*, reads four bytes (32-bits) from port
* *void outl(unsigned long word, int port)* write four bytes (32-bits) to port

The port argument specifies the address of the port where the reads or
writes are done, and its type is platform dependent (may be unsigned
long or unsigned short).

Some devices may have problems when the processor is trying to
transfer data too fast to and from the device. To avoid this issue we
may need to insert a delay after an I/O operation and there functions
you can use that introduce this delay. Their names are similar to
those described above, with the exception that it ends in _p: inb_p,
outb_p, etc.

For example, the following sequence writes a byte on COM1 serial port
and then reads it:

.. code-block:: c

   #include <asm/io.h>
   #define MY_BASEPORT 0x3F8

   unsigned char value = 0xFF;
   outb(value, MY_BASEPORT);
   value = inb(MY_BASEPORT);

Accessing I/O ports from userspace
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Although the functions described above are defined for device drivers,
they can also be used in user space by including the <sys/io.h>
header. In order to be used, ioperm or iopl must first be called to
get permission to perform port operations. The ioperm function obtains
permission for individual ports, while iopl for the entire I/O address
space. To use these features, the user must be root.

The following sequence used in user space gets permission for the
first 3 ports of the serial port, and then releases them:

.. code-block:: c

   #include <sys/io.h>
   #define MY_BASEPORT 0x3F8

   if (ioperm(MY_BASEPORT, 3, 1)) {
	/* handle error */
   }
		
   if (ioperm(MY_BASEPORT, 3, 0)) {
	/* handle error */
   }
		
The third parameter of the ioperm function is used to request or
release port permission: 1 to get permission and 0 to release.

Interrupt handling in Linux
===========================

Requesting an interrupt
-----------------------

As with other resources, a driver must gain access to an interrupt
line before it can use it and release it at the end of the execution.

In Linux, the request to obtain and release an interrupt is done using
the :c:func:`requests_irq` and :c:func:`free_irq` functions:

.. code-block:: c

   #include <linux/interrupt.h>

   typedef irqreturn_t (*irq_handler_t)(int, void *);

   int request_irq(unsigned int irq_no, irq_handler_t handler,
   unsigned long flags, const char *dev_name, void *dev_id);

   void free_irq(unsigned int irq_no, void *dev_id);

Note that to get an interrupt, the developer calls
:c:func:`request_irq`. When calling this function you must specify the
interrupt number (*irq_no*), a handler that will be called when the
interrupt is generated (*handler*), flags that will instruct the
kernel about the desired behaviour (*flags*), the name of the device
using this interrupt (*dev_name*), and a pointer that can be
configured by the user at any value, and that has no global
significance (*dev_id*). Most of the time, *dev_id* will be
pointer to the device driver's private data. When the interrupt is
released, using the :c:func:`free_irq` function, the developer must
send the same pointer value (*dev_id*) along with the same interrupt
number (*irq_no*). The device name (*dev_name*) is used to display
statistics in */proc/interrupts*.

The value that :c:func:`request_irq` returns is 0 if the entry was
successful or a negative error code indicating the reason for the
failure. A typical value is *-EBUSY* which means that the interrupt
was already requested by another device driver.

The *handler* function is executed in interrupt context which means
that we can't call blocking APIs such as :c:func:`mutex_lock` or
:c:func:`msleep`. We must also avoid doing a lot of work in the
interrupt handler and instead use deferred work if needed. The actions
performed in the interrupt handler include reading the device
registers to get the status of the device and acknowledge the
interrupt, operations that most of the time can be performed with
non-blocking calls.

There are situations where although a device uses interrupts we can't
read the device's registers in a non-blocking mode (for example a
sensor connected to an I2C or SPI bus whose driver does not guarantee
that bus read / write operations are non-blocking ). In this
situation, in the interruption, we must plan a work-in-process action
(work queue, kernel thread) to access the device's registers. Because
such a situation is relatively common, the kernel provides the
:c:func:`request_threaded_irq` function to write interrupt handling
routines running in two phases: a process-phase and an interrupt
context phase:

.. code-block:: c

   #include <linux/interrupt.h>
		
   int request_threaded_irq(unsigned int irq, irq_handler_t handler,
		            irq_handler_t thread_fn,
			    unsigned long flags, const char *name, void *dev);

*handler* is the function running in interrupt context, and will
implement critical operations while the thread_fn function runs in
process context and implements the rest of the operations.

The flags that can be transmitted when an interruption is made are:

* *IRQF_SHARED* announces the kernel that the interrupt can be
  shared with other devices. If this flag is not set, then if there is
  already a handler associated with the requested interrupt, the
  request for interrupt will fail. A shared interrupt is handled in a
  special way by the kernel: all of the associated interrupt handlers
  will be executed until the device that generated the interrupt will
  be identified. But how can a device driver know if the interrupt
  handling routine was activated by an interrupt generated by the
  device it manages? Virtually all devices that offer interrupt
  support have a status register that can be interrogated in the
  handling routine to see if the interrupt was or was not generated by
  the device (for example, in the case of the 8250 serial port, this
  status register is IIR - Interrupt Information Register). When
  requesting a shared interrupt, the dev_id argument must be unique
  and it must not be NULL.  Usually it is set to module's private
  data.

* *IRQF_ONESHOT* interrupt will be reactivated after running the process
  context routine; Without this flag, the interrupt will be
  reactivated after running the handler routine in the context of
  the interrupt


Requesting the interrupt can be done either at the initialization of
the driver (:c:func:`init_module`), when the device is probed, or when
the device is used (e.g. during *open*).

The following example performs the interrupt request for the COM1
serial port:

.. code-block:: c

   #include <linux/interrupt.h>

   #define MY_BASEPORT 0x3F8
   #define MY_IRQ 4

   static my_init(void)
   {
	[...]
	struct my_device_data *my_data;
	int err;

	err = request_irq(MY_IRQ, my_handler, IRQF_SHARED,
		          "com1", my_data);
	if (err < 0) {
	    /* handle error*/
	    return err;
	}
	[...]
   }

As you can see, the IRQ for serial port COM1 is 4, which is used in
shared mode (IRQF_SHARED).

.. attention:: When requesting a shared interrupt (IRQF_SHARED) the
	       *dev_id* argument can not be NULL.

To release the interrupt associated with the serial port, the
following operations will be executed:

.. code-block:: c

   free_irq (MY_IRQ, my_data);



During the initialization functiom (c:func:`init_module`), or in the
function that opens the device, interrupts must be activated for the
device. This operation is dependent on the device, but most often
involves setting a bit from the control register.


As an example, for the 8250 serial port, the following operations must
be performed to enable interrupts:

.. code-block:: c

   #include <asm/io.h>
   #define MY_BASEPORT 0x3F8

   outb(0x08, MY_BASEPORT+4);
   outb(0x01, MY_BASEPORT+1);


In the above example, two operations are performed:
   
1. All interruptions are activated by setting bit 3 (Aux Output 2) in
   the MCR register - Modem Control Register
2. The RDAI (Transmit Holding Register Empty Interrupt) is activated
   by setting the appropriate bit in the IER - Interrupt Enable
   Register.


Implementing an interrupt handler
---------------------------------

Lets take a look at the signature of the interrupt handler function:

.. code-block:: c

   irqreturn_t (*handler)(int irq_no, void *dev_id);

The function receives as parameters the number of the interrupt
(*irq_no*) and the pointer sent to :c:func:`request_irq` when the
interrupt was requested. The interrupt handling routine must return a
value with a type of :c:type:`typedef irqreturn_t`. For the current kernel
version, there are three valid values: *IRQ_NONE*, *IRQ_HANDLED*,
and *IRQ_WAKE_THREAD*. The device driver must return *IRQ_NONE* if
it notices that the interrupt has not been generated by the device it
is in charge. Otherwise, the device driver must return *IRQ_HANDLED*
if the interrupt can be handled directly from the interrupt context or
*IRQ_WAKE_THREAD* to schedule the running of the process context
processing function.

The skeleton for an interrupt handler is:

.. code-block:: c

   irqreturn_t my_handler(int irq_no, void *dev_id)
   {
       struct my_device_data *my_data = (struct my_device_data *) dev_id;

       /* if interrupt is not for this device (shared interrupts) */
           /* return IRQ_NONE;*/

       /* clear interrupt-pending bit */
       /* read from device or write to device*/

       return IRQ_HANDLED;
   }


Typically, the first thing executed in the interrupt handler is to
determine whether the interrupt was generated by the device that the
driver ordered. This usually reads information from the device's
registers to indicate whether the device has generated an
interrupt. The second thing is to reset the interrupt pending bit on
the physical device as most devices will no longer generate
interruptions until this bit has been reset (e.g. for the 8250
serial port bit 0 in the IIR register must be cleared).


Locking
-------

Because the interrupt handlers run in interrupt context the actions
that can be performed are limited: unable to access user space memory,
can't call blocking functions. Also synchronization using spinlocks is
tricky and can lead to deadlocks if the spinlock used is already
acquired by a process that has been interrupted by the running
handler.

However, there are cases where device drivers have to synchronize
using interrupts, such as when data is shared between the interrupt
handler and process context or bottom-half handlers. In these
situations it is necessary to both deactivate the interrupt and use
spinlocks.

There are two ways to disable interrupts: disabling all interrupts, at
the processor level, or disabling a particular interrupt at the device
or interrupt controller level. Processor disabling is faster and is
therefore preferred. For this purpose, there are locking functions
that disable and enable interrupts acquiring and release a spinlock at
the same time: :c:func:`spin_lock_irqsave`,
:c:func:`spin_unlock_irqrestore`, :c:func:`spin_lock_irq`, and
:c:func:`spin_unlock_irq`:

.. code-block:: c

   #include <linux/spinlock.h>

   void spin_lock_irqsave (spinlock_t * lock, unsigned long flags);
   void spin_unlock_irqrestore (spinlock_t * lock, unsigned long flags);

   void spin_lock_irq (spinlock_t * lock);
   void spin_unlock_irq (spinlock_t * lock);

The :c:func:`spin_lock_irqsave` function disables interrupts for the
local processor before it obtains the spinlock; The previous state of
the interrupts is saved in *flags*.

If you are absolutely sure that the interrupts on the current
processor have not already been disabled by someone else and you are
sure you can activate the interrupts when you release the spinlock,
you can use :c:func:`spin_lock_irq`.

For read / write spinlocks there are similar functions available:

* :c:func:`read_lock_irqsave`
* :c:func:`read_unlock_irqrestore`
* :c:func:`read_lock_irq`
* :c:func:`read_unlock_irq`
* :c:func:`write_lock_irqsave`
* :c:func:`write_unlock_irqrestore`
* :c:func:`write_lock_irq`
* :c:func:`write_unlock_irq`
    
If we want to disable interrupts at the interrupt controller level
(not recommended because disabling a particular interrupt is slower,
we can not disable shared interrupts) we can do this with
:c:func:`disable_irq`, :c:func:`disable_irq_nosync`, and
:c:func:`enable_irq`. Using these functions disabled the interrupts on
all processors. Calls can be nested: if disable_irq is called twice,
it will require as many calls enable_irq to enable it. The difference
between disable_irq and disable_irq_nosync is that the first one will
wait for the executed handlers to finish. Because of this,
:c:func:`disable_irq_nosync` is generally faster, but may lead to
races with the interrupts handler, so when not sure use
:c:func:`disable_irq`.

The following sequence disables and then enables the interrupt for
the COM1 serial port:

.. code-block:: c

   #define MY_IRQ 4

   disable_irq (MY_IRQ);
   enable_irq (MY_IRQ);

It is also possible to disable interrupts at the device level. This
approach is also slower than disabling interrupts at the processor
level but it works with shared interrupts. The way to accomplish this
is device specific and it usually means we have to clear a bit from
one of the control registers.

It is also possible to disable all interrupts for the current
processor independent of taking locks. Disabling all interruptions by
device drivers for synchronization purposes is inappropriate because
races are still possible if the interrupt is handled on another
CPU. For reference, the functions that disable / enable interrupts on
the local processor are :c:func:`local_irq_disable` and
:c:func:`local_irq_enable`.

In order to use a resource shared between process context and the
interrupt handling routine, the functions described above will be used
as follows:

.. code-block:: c

   static spinlock_t lock;
   
   / * IRQ handling routine: interrupt context * /
   irqreturn_t so2_kbd_interrupt_handle(int irq_no, void * dev_id)
   {
       ...
       spin_lock(&lock);
       / * Critical region - access shared resource */
       spin_unlock (&lock);
       ...
   }
   
   / * Process context: Disable interrupts when locking * /
   static void my_access(void)
   {
       unsigned long flags;
   
       spin_lock_irqsave(&lock, flags);
       /* Critical region - access shared resource */
       spin_unlock_irqrestore(&lock, flags);
       
       ...
   }

   void my_init (void)
   {
       ...
       spin_lock_init (&lock);
       ...
   }

		
The *my_access function* above runs in process context. To
synchronize access to the share data, we disable the interrupts and
use the spinlock *lock*, i.e. the :c:func:`spin_lock_irqsave` and
:c:func:`spin_unlock_irqrestore` functions.

In the interrupt handling routine, we use the :c:func:`spin_lock` and
:c:func:`spin_unlock` functions to access the shared resource.

.. note:: The *flags* argument for :c:func:`spin_lock_irqsave` and
	  :c:func:`spin_unlock_irqrestore` is a value and not a pointer but keep
	  in mind that :c:func:`spin_lock_irqsave` function changes the value of
	  the flag, since this is actually a macro.

Interrupt statistics
--------------------

Information and statistics about system interrupts can be found in
*/proc/interrupts* or */proc/stat*. Only system interrupts with
associated interrupt handlers appear in */proc/interrupts*:

.. code-block:: shell

   # cat /proc/interrupts
		   CPU0
   0:           7514294       IO-APIC-edge   timer
   1:              4528       IO-APIC-edge   i8042
   6:                 2       IO-APIC-edge   floppy
   8:                 1       IO-APIC-edge   rtc
   9:                 0       IO-APIC-level  acpi
   12:             2301       IO-APIC-edge   i8042
   15:               41       IO-APIC-edge   ide1
   16:             3230       IO-APIC-level  ioc0
   17:             1016       IO-APIC-level  vmxnet ether
   NMI:               0
   LOC:         7229438
   ERR:               0
   MIS:               0

The first column specifies the IRQ associated with the interrupt. The
following column shows the number of interrupts that were generated
for each processor in the system; The last two columns provide
information about the interrupt controller and the device name that
registered the handler for that interrupt.

The */proc/state* file provides information about system activity,
including the number of interruptions generated since the last (re)boot
of the system:

.. code-block:: shell
		
   # cat /proc/stat | grep in
   intr 7765626 7754228 4620 0 0 0 0 2 0 1 0 0 0 2377 0 0 41 3259 1098 0 0 0 0 0 0 0 0 0
   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

Each line in the */proc/state* file begins with a keyword that
specifies the meaning of the information on the line. For information
on interrupts, this keyword is intr. The first number on the line
represents the total number of interrupts, and the other numbers
represent the number of interrupts for each IRQ, starting at 0. The
counter includes the number of interrupts for all processors in the
system.


Further reading
===============

Serial Port
-----------

* `Serial Port <http://en.wikipedia.org/wiki/Serial_port>`_
* `Interfacing the Serial / RS232 Port <http://www.beyondlogic.org/serial/serial.htm>`_


Parallel port
-------------

* `Interfacing the Standard Parallel Port <http://www.beyondlogic.org/spp/parallel.htm>`_
* `Parallel Port Central <http://www.lvr.com/parport.htm>`_

Keyboard controller
-------------------

* `Intel 8042 <http://en.wikipedia.org/wiki/Intel_8042>`_
* drivers/input/serio/i8042.c 
* drivers/input/keyboard/atkbd.c

Linux device drivers
--------------------

* `Linux Device Drivers, 3rd ed., Ch. 9 - Communicating with Hardware <http://lwn.net/images/pdf/LDD3/ch09.pdf>`_
* `Linux Device Drivers, 3rd ed., Ch. 10 - Interrupt Handling <http://lwn.net/images/pdf/LDD3/ch10.pdf>`_
* `Interrupt Handlers <http://tldp.org/LDP/lkmpg/2.6/html/x1256.html>`_
