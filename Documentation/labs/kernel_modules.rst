==============
Kernel modules
==============

.. slideconf::
   :theme: single-level
   :autoslides: false

.. slide:: Kernel modules
   :level: 1

Lab objectives
==============

* creating simple modules
* describing the process of kernel module compilation
* presenting how a module can be used with a kernel
* simple kernel debugging methods

.. slide:: What is a kernel module?
   :level: 2

   * object file (``*.ko``)
   * extends functionality of the Linux kernel
   * Y vs M
   * can be loaded at runtime
   * insmod/rmmod
   * modprobe

Overview
========

A monolithic kernel, though faster than a microkernel, has the disadvantage of
lack of modularity and extensibility. On modern monolithic kernels, this has
been solved by using kernel modules. A kernel module (or loadable kernel mode)
is an object file that contains code that can extend the kernel functionality
at runtime (it is loaded as needed); When a kernel module is no longer needed,
it can be unloaded. Most of the device drivers are used in the form of kernel
modules.

For the development of Linux device drivers, it is recommended to download the
kernel sources, configure and compile them and then install the compiled version
on the test / development tool machine.

An example of kernel module
===========================

Below is a very simple example of kernel module. When loading into the kernel,
it will generate the message "Hi". When unloading the kernel module, the "Bye"
message will be generated.

.. slide:: Hello World!
   :level: 2

   .. code-block:: c
   
       #include <linux/kernel.h>
       #include <linux/init.h>
       #include <linux/module.h>
        
       MODULE_DESCRIPTION("Simple hello world kernel module");
       MODULE_AUTHOR("Me");
       MODULE_LICENSE("GPL");
        
       static int dummy_init(void)
       {
               pr_debug("Hi\n");
               return 0;
       }
       static void dummy_exit(void)
       {
               pr_debug("Bye\n");
       }
        
       module_init(dummy_init);
       module_exit(dummy_exit);

.. code-block:: c

    #include <linux/kernel.h>
    #include <linux/init.h>
    #include <linux/module.h>
     
    MODULE_DESCRIPTION("My kernel module");
    MODULE_AUTHOR("Me");
    MODULE_LICENSE("GPL");
     
    static int dummy_init(void)
    {
            pr_debug("Hi\n");
            return 0;
    }
     
    static void dummy_exit(void)
    {
            pr_debug("Bye\n");
    }
     
    module_init(dummy_init);
    module_exit(dummy_exit);


The generated messages will not be displayed on the console but will be saved
in a specially reserved memory area for this, from where they will be extracted
by the logging daemon (syslog). To display kernel messages, you can use the dmesg
command or inspect the logs:

.. code-block:: bash
   
   # cat /var/log/syslog | tail -2
   Feb 20 13:57:38 asgard kernel: Hi
   Feb 20 13:57:43 asgard kernel: Bye
    
   # dmesg | tail -2
   Hi
   Bye

Compiling kernel modules
========================

Compiling a kernel module differs from compiling an user program. First, other
headers should be used. Also, the module should not be linked to libraries.
And, last but not least, the module must be compiled with the same options as
the kernel in which we load the module. For these reasons, there is a standard
compilation method (kbuild). This method requires the use of two files:
a Makefile and a Kbuild file.

Below is an example of a Makefile:

.. slide:: Kernel module compilation
   :level: 2

   * use ``Kbuild`` build system
   * **in tree** vs **out of tree**

   .. code-block:: bash
      
      # Makefile
      KDIR = /lib/modules/`uname -r`/build
       
      kbuild:
              make -C $(KDIR) M=`pwd`
       
      clean:
              make -C $(KDIR) M=`pwd` clean

   .. code-block:: bash

      # Kbuild
      EXTRA_CFLAGS = -Wall -g
      obj-m        = modul.o

.. code-block:: bash
   
   KDIR = /lib/modules/`uname -r`/build
    
   kbuild:
           make -C $(KDIR) M=`pwd`
    
   clean:
           make -C $(KDIR) M=`pwd` clean

And the example of a Kbuild file used to compile a module:

.. code-block:: bash
   
   EXTRA_CFLAGS = -Wall -g
    
   obj-m        = modul.o
   

As you can see, making the Makefile file in the example shown will result in 
the make invocation in the kernel source directory (``/lib/modules/`uname -r`/build``)
and referring to the current directory (``M = `pwd```). This process ultimately
leads to reading the Kbuild file from the current directory  and compiling
the module as instructed in this file.

For labs we will configure different KDIR, according to the virtual machine
specifications:

.. code-block:: bash
   
   KDIR = /usr/src/linux-so2
   [...]

A Kbuild file contains one or more directives for compiling a kernel module.
The easiest example of such a directive is ``obj-m = modul.o``. Following this
directive, a kernel module (ko - kernel object) will be created,
starting from the ``module.o`` file. ``module.o`` will be created starting from
``module.c`` or ``module.S``. All of these files can be found in the Kbuild's
directory.

An example of a Kbuild file that uses several sub-modules is shown below:

.. code-block:: bash
   
   EXTRA_CFLAGS = -Wall -g
    
   obj-m        = supermodul.o
   supermodul-y = module-a.o module-b.o

For the example above, the steps to compile are:
   
   * compile the module-a.c and module-b.c source, resulting in module-a.o and
     module-b.o objects
   * module-a.o and module-b.o will then be linked in supermodule.o
   * from supermodul.o will create supermodul.ko module


The suffix of targets in Kbuild determines how they are used, as follows:

   * M (modules) is a target for loadable kernel modules
   * Y (yes) represents a target for object files to be compiled and then linked
     to a module (``$(mode_name)-y``) or within the kernel (``obj-y``)
   * any other target suffix will be ignored by Kbuild and will not be compiled


These suffixes are used to easily configure the kernel by running the ``make
menuconfig`` command or directly editing the .config file. This file sets a
series of variables that are used to determine which features are added to the
kernel at build time. For example, when adding BTRFS support with
``make menuconfig``, add the line CONFIG_BTRFS_FS = y to the .config file.
The BTRFS kbuild contains the line ``obj-$(CONFIG_BTRFS_FS):= btrfs.o``, which
becomes ``obj-y:= btrfs.o``. This will compile the btrfs.o object and will be
linked to the kernel. Before the variable was set, the line became ``obj:=btrfs.o``
and so it was ignored, and the kernel was build-at without BTRFS support.

For more details, see the ``makefiles.txt`` file and the ``modules.txt`` file within
the kernel sources.

Loading/unloading a kernel module
=================================

To load a kernel module, use the insmod utility. This utility receives as a
parameter the path to the .ko file in which the module was compiled and linked.
Unloading the module from the kernel is done using the rmmod command, which receives
the module name as a parameter.

.. code-block:: bash
   
   $ insmod module.ko
   $ rmmod module.ko

When loading the kernel module, the routine specified as a parameter of the
``module_init`` macro will be executed. Similarly, when the module is unloaded
the routine specified as a parameter of the ``module_exit`` will be executed.

A complete example of compiling and loading/unloading mode is presented below:

.. code-block:: bash
   
   faust:~/lab-01/modul-lin# ls
   Kbuild  Makefile  modul.c
    
   faust:~/lab-01/modul-lin# make
   make -C /lib/modules/`uname -r`/build M=`pwd`
   make[1]: Entering directory `/usr/src/linux-2.6.28.4'
     LD      /root/lab-01/modul-lin/built-in.o
     CC [M]  /root/lab-01/modul-lin/modul.o
     Building modules, stage 2.
     MODPOST 1 modules
     CC      /root/lab-01/modul-lin/modul.mod.o
     LD [M]  /root/lab-01/modul-lin/modul.ko
   make[1]: Leaving directory `/usr/src/linux-2.6.28.4'
    
   faust:~/lab-01/modul-lin# ls
   built-in.o  Kbuild  Makefile  modul.c  Module.markers
   modules.order  Module.symvers  modul.ko  modul.mod.c
   modul.mod.o  modul.o
    
   faust:~/lab-01/modul-lin# insmod modul.ko
    
   faust:~/lab-01/modul-lin# dmesg | tail -1
   Hi
    
   faust:~/lab-01/modul-lin# rmmod modul
    
   faust:~/lab-01/modul-lin# dmesg | tail -2
   Hi
   Bye

Information about modules loaded into the kernel can be found using the lsmod
command or by inspecting the ``/proc/modules``, ``/sys/module`` directories.

Debugging
=========

.. slide:: printk
   :level: 2

   * more or less similar with printf

   .. code-block:: bash

      printk(KERN_DEBUG "Hello!\n");
      pr_debug("Hello!\n");

   * 8 log levels (0 - KERN_EMERG, 7 - KERN_DEBUG)
   * console, kernel buffer, dmesg

   .. code-block:: bash

      $ cat /proc/sys/kernel/printk
              7       4       1       7
              current default minimum boot-time-default

.. slide:: Debugging
   :level: 2

   .. code-block:: bash

      BUG: unable to handle kernel paging request at 00001234
      IP: [<c89d4005>] my_oops_init+0x5/0x20 [oops]


   * oops vs panic

   .. code-block:: bash

      $ cat /proc/modules
      oops 1280 1 - Loading 0xc89d4000

   .. code-block:: bash

      $ objdump -dS --adjust-vma=0xc89d4000 oops.ko

   .. code-block:: bash

      $ addr2line -e oops.o 0x5

Troubleshooting a kernel module is much more complicated than debugging a
regular program. First, a mistake in a kernel module can lead to blocking the
entire system. Troubleshooting is therefore much slowed down. To avoid reboot,
it is recommended to use a virtual machine (qemu, virtualbox, vmware).

When a module containing bugs is inserted into the kernel, it will eventually
generate a `kernel oops <https://en.wikipedia.org/wiki/Linux_kernel_oops>`_.
A kernel oops is an invalid operation detected by the kernel and can only
be generated by the kernel. For a stable kernel version, it almost certainly 
means that the module contains a bug. After the oops appears, the kernel will
continue to work.

Very important to the appearance of a kernel oops is saving the generated
message. As noted above, messages generated by the kernel are saved in logs and
can be displayed with the dmesg command. To make sure that no kernel message
is lost, it is recommended to insert/test the kernel directly from the console,
or periodically check the kernel messages. Noteworthy is that an oops can occur
because of a programming error, but also a hardware error.

If a fatal error occurs, after which the system can not return to a stable
state, a panic kernel is generated.

Look at the kernel module below that contains a bug to generate an oops:

.. code-block:: c
    
    /*
     * Oops generating kernel module
     */
     
    #include <linux/kernel.h>
    #include <linux/module.h>
    #include <linux/init.h>
     
    MODULE_DESCRIPTION ("Oops");
    MODULE_LICENSE ("GPL");
    MODULE_AUTHOR ("PSO");
     
    #define OP_READ         0
    #define OP_WRITE        1
    #define OP_OOPS         OP_WRITE
     
    static int my_oops_init (void)
    {
            int *a;
     
            a = (int *) 0x00001234;
    #if OP_OOPS == OP_WRITE
            *a = 3;
    #elif OP_OOPS == OP_READ
            printk (KERN_ALERT "value = %d\n", *a);
    #else
    #error "Unknown op for oops!"
    #endif
     
            return 0;
    }
     
    static void my_oops_exit (void)
    {
    }
     
    module_init (my_oops_init);
    module_exit (my_oops_exit);

.. **

Inserting this module into the kernel will generate an oops:

.. code-block:: bash
   
   faust:~/lab-01/modul-oops# insmod oops.ko
   [...]
   
   faust:~/lab-01/modul-oops# dmesg | tail -32
   BUG: unable to handle kernel paging request at 00001234
   IP: [<c89d4005>] my_oops_init+0x5/0x20 [oops]
     *de = 00000000
   Oops: 0002 [#1] PREEMPT DEBUG_PAGEALLOC
   last sysfs file: /sys/devices/virtual/net/lo/operstate
   Modules linked in: oops(+) netconsole ide_cd_mod pcnet32 crc32 cdrom [last unloaded: modul]
   
   Pid: 4157, comm: insmod Not tainted (2.6.28.4 #2) VMware Virtual Platform
   EIP: 0060:[<c89d4005>] EFLAGS: 00010246 CPU: 0
   EIP is at my_oops_init+0x5/0x20 [oops]
   EAX: 00000000 EBX: fffffffc ECX: c89d4300 EDX: 00000001
   ESI: c89d4000 EDI: 00000000 EBP: c5799e24 ESP: c5799e24
    DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 0068
   Process insmod (pid: 4157, ti=c5799000 task=c665c780 task.ti=c5799000)
   Stack:
    c5799f8c c010102d c72b51d8 0000000c c5799e58 c01708e4 00000124 00000000
    c89d4300 c5799e58 c724f448 00000001 c89d4300 c5799e60 c0170981 c5799f8c
    c014b698 00000000 00000000 c5799f78 c5799f20 00000500 c665cb00 c89d4300
   Call Trace:
    [<c010102d>] ? _stext+0x2d/0x170
    [<c01708e4>] ? __vunmap+0xa4/0xf0
    [<c0170981>] ? vfree+0x21/0x30
    [<c014b698>] ? load_module+0x19b8/0x1a40
    [<c035e965>] ? __mutex_unlock_slowpath+0xd5/0x140
    [<c0140da6>] ? trace_hardirqs_on_caller+0x106/0x150
    [<c014b7aa>] ? sys_init_module+0x8a/0x1b0
    [<c0140da6>] ? trace_hardirqs_on_caller+0x106/0x150
    [<c0240a08>] ? trace_hardirqs_on_thunk+0xc/0x10
    [<c0103407>] ? sysenter_do_call+0x12/0x43
   Code: <c7> 05 34 12 00 00 03 00 00 00 5d c3 eb 0d 90 90 90 90 90 90 90 90
   EIP: [<c89d4005>] my_oops_init+0x5/0x20 [oops] SS:ESP 0068:c5799e24
   ---[ end trace 2981ce73ae801363 ]---

Although relatively cryptic, the message provided by the kernel to the 
appearance of an oops provides valuable information about the error. First line:

.. code-block:: bash
   
   BUG: unable to handle kernel paging request at 00001234
   EIP: [<c89d4005>] my_oops_init + 0x5 / 0x20 [oops]

Tells us the cause and the address of the instruction that generated the error.
In our case this is an invalid access to memory.

Next line

   ``Oops: 0002 [# 1] PREEMPT DEBUG_PAGEALLOC``

Tells us that it's the first oops (#1). This is important in the context that
an oops can lead to other oopses. Usually only the first oops is relevant.
Furthermore, the oops code (0002) provides information about the error type
(in memory manager -> fault.c ):

   * Bit 0 == 0 means no page found, 1 means protection fault
   * Bit 1 == 0 means read, 1 means write
   * Bit 2 == 0 means kernel, 1 means user - mode 

In this case, we have a write access that generated the oops (bit 1 is 1).

Below is a dump of the registers. It decodes the instruction pointer (EIP)
value and notes that the bug appeared in the my_oops_init function with a
5-byte offset (``EIP: [<c89d4005>] my_oops_init+0x5``). The message also shows
the stack content and a backtrace of calls until then.

If an invalid read call is generated ( ``#define OP_OOPS OP_READ``), the message
will be the same, but the oops code will differ, which would now be 0000 :

.. code-block:: bash
   
   faust:~/lab-01/modul-oops# dmesg | tail -33
   BUG: unable to handle kernel paging request at 00001234
   IP: [<c89c3016>] my_oops_init+0x6/0x20 [oops]
     *de = 00000000
   Oops: 0000 [#1] PREEMPT DEBUG_PAGEALLOC
   last sysfs file: /sys/devices/virtual/net/lo/operstate
   Modules linked in: oops(+) netconsole pcnet32 crc32 ide_cd_mod cdrom
   
   Pid: 2754, comm: insmod Not tainted (2.6.28.4 #2) VMware Virtual Platform
   EIP: 0060:[<c89c3016>] EFLAGS: 00010292 CPU: 0
   EIP is at my_oops_init+0x6/0x20 [oops]
   EAX: 00000000 EBX: fffffffc ECX: c89c3380 EDX: 00000001
   ESI: c89c3010 EDI: 00000000 EBP: c57cbe24 ESP: c57cbe1c
    DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 0068
   Process insmod (pid: 2754, ti=c57cb000 task=c66ec780 task.ti=c57cb000)
   Stack:
    c57cbe34 00000282 c57cbf8c c010102d c57b9280 0000000c c57cbe58 c01708e4
    00000124 00000000 c89c3380 c57cbe58 c5db1d38 00000001 c89c3380 c57cbe60
    c0170981 c57cbf8c c014b698 00000000 00000000 c57cbf78 c57cbf20 00000580
   Call Trace:
    [<c010102d>] ? _stext+0x2d/0x170
    [<c01708e4>] ? __vunmap+0xa4/0xf0
    [<c0170981>] ? vfree+0x21/0x30
    [<c014b698>] ? load_module+0x19b8/0x1a40
    [<c035d083>] ? printk+0x0/0x1a
    [<c035e965>] ? __mutex_unlock_slowpath+0xd5/0x140
    [<c0140da6>] ? trace_hardirqs_on_caller+0x106/0x150
    [<c014b7aa>] ? sys_init_module+0x8a/0x1b0
    [<c0140da6>] ? trace_hardirqs_on_caller+0x106/0x150
    [<c0240a08>] ? trace_hardirqs_on_thunk+0xc/0x10
    [<c0103407>] ? sysenter_do_call+0x12/0x43
   Code: <a1> 34 12 00 00 c7 04 24 54 30 9c c8 89 44 24 04 e8 58 a0 99 f7 31
   EIP: [<c89c3016>] my_oops_init+0x6/0x20 [oops] SS:ESP 0068:c57cbe1c
   ---[ end trace 45eeb3d6ea8ff1ed ]---

objdump
-------

Detailed information about the instruction that generated the oops can be found
using the objdump utility. Useful options to use are ``-d`` to disassemble the
code and ``-S`` for interleaving code C in assembly language code.
For efficient decoding, however, we need the address where the kernel module was
loaded. This can be found in /proc/modules.

Here's an example of using objdump on the above module to identify the instruction
that generated the oops:

.. code-block:: bash
   
   faust:~/lab-01/modul-oops# cat /proc/modules
   oops 1280 1 - Loading 0xc89d4000
   netconsole 8352 0 - Live 0xc89ad000
   pcnet32 33412 0 - Live 0xc895a000
   ide_cd_mod 34952 0 - Live 0xc8903000
   crc32 4224 1 pcnet32, Live 0xc888a000
   cdrom 34848 1 ide_cd_mod, Live 0xc886d000
   
   faust:~/lab-01/modul-oops# objdump -dS --adjust-vma=0xc89d4000 oops.ko
   
   oops.ko:     file format elf32-i386
   
   
   Disassembly of section .text:
   
   c89d4000 <init_module>:
   #define OP_READ         0
   #define OP_WRITE        1
   #define OP_OOPS         OP_WRITE
   
   static int my_oops_init (void)
   {                             
   c89d4000:       55                      push   %ebp
   #else                                              
   #error "Unknown op for oops!"                      
   #endif
   
           return 0;
   }
   c89d4001:       31 c0                   xor    %eax,%eax
   #define OP_READ         0
   #define OP_WRITE        1
   #define OP_OOPS         OP_WRITE
   
   static int my_oops_init (void)
   {
   c89d4003:       89 e5                   mov    %esp,%ebp
           int *a;
   
           a = (int *) 0x00001234;
   #if OP_OOPS == OP_WRITE
           *a = 3;
   c89d4005:       c7 05 34 12 00 00 03    movl   $0x3,0x1234
   c89d400c:       00 00 00
   #else
   #error "Unknown op for oops!"
   #endif
   
           return 0;
   }
   c89d400f:       5d                      pop    %ebp
   c89d4010:       c3                      ret
   c89d4011:       eb 0d                   jmp    c89c3020 <cleanup_module>
   c89d4013:       90                      nop
   c89d4014:       90                      nop
   c89d4015:       90                      nop
   c89d4016:       90                      nop
   c89d4017:       90                      nop
   c89d4018:       90                      nop
   c89d4019:       90                      nop
   c89d401a:       90                      nop
   c89d401b:       90                      nop
   c89d401c:       90                      nop
   c89d401d:       90                      nop
   c89d401e:       90                      nop
   c89d401f:       90                      nop
   
   c89d4020 <cleanup_module>:
   
   static void my_oops_exit (void)
   {
   c89d4020:       55                      push   %ebp
   c89d4021:       89 e5                   mov    %esp,%ebp
   }
   c89d4023:       5d                      pop    %ebp
   c89d4024:       c3                      ret
   c89d4025:       90                      nop
   c89d4026:       90                      nop
   c89d4027:       90                      nop

Note that the instruction that generated the oops (``c89d4005`` identified
earlier) is:

  ``C89d4005: c7 05 34 12 00 00 03 movl $ 0x3,0x1234``

That is exactly what was expected - storing value 3 at 0x0001234.

The /proc/modules is used to find the address where a kernel module is loaded.
The --adjust-vma option allows you to display instructions relative to
``0xc89d4000``. The ``-l`` option displays the number of each line in the source code
interleaved with the assembly language code.

addr2line
---------

A more simplistic way to find the code that generated an oops is to use the
addr2line utility:

.. code-block:: bash

   faust:~/lab-01/modul-oops# addr2line -e oops.o 0x5
   /root/lab-01/modul-oops/oops.c:23

Where ``0x5`` is the value of the program counter (``EIP = c89d4005``) that generated the
oops, minus the base address of the module (``0xc89c4000``) according to ``/proc/modules``

minicom
-------

Minicom (or other equivalent utilities, eg ``picocom``, ``screen``) is a utility that
can be used to connect and interact with a serial port. The serial port is the
basic method for analyzing kernel messages or interacting with an embedded
system in the development phase. There are two more common ways to connect:

* a serial serial port where the device we are going to use is ``/dev/ttyS0``
* a serial USB port (FDTI) in which case the device we are going to use is ``/dev/ttyUSB``.

For the virtual machine used in the lab, the device that we need to use is 
displayed after the virtual machine starts:

``char device redirected to /dev/pts/20 (label virtiocon0)``

Minicom use:

.. code-block:: bash
   
   #for connecting via COM1 and using a speed of 115,200 characters per second
   minicom -b 115200 -D /dev/ttyS0
   
   #For USB serial port connection
   minicom -D /dev/ttyUSB0
   
   #To connect to the serial port of the virtual machine
   minicom -D /dev/pts/20

netconsole
----------

Netconsole is a utility that allows logging of kernel debugging messages over
the network. This is useful when the disk logging system does not work when
serial ports are not available or when the terminal does not respond to
commands. Netconsole comes in the form of a kernel module.

To work, it needs the following parameters:

   * port, IP address, and the source interface name of the debug station
   * port, MAC address, and IP address of the machine to which the debug
     messages will be sent 

These parameters can be configured when the module is inserted into the kernel,
or even while the module is inserted if it has been compiled with the
CONFIG_NETCONSOLE_DYNAMIC option.

An example configuration when inserting is as follows:

.. code-block:: bash
   
   alice:~# modprobe netconsole netconsole=6666@192.168.191.130/eth0,6000@192.168.191.1/00:50:56:c0:00:08

Thus, the debug messages on the station that has the address 192.168.191.130
will be sent to the eth0 interface, having source port 6666. The messages will
be sent to 192.168.191.1 with the MAC address 00: 50: 56: c0: 00: 08, on port
6000.

Messages can be played on the destination station using netcat :

.. code-block:: bash

   bob:~ # nc -l -p 6000 -u

Alternatively, the destination station can configure syslogd to intercept these
messages. More information can be found here .

Printk debugging
----------------

``The two oldest and most useful debugging aids are Your brain and Printf``

For debugging, a primitive way is often used, but it is quite effective: printk
debugging. Although a debugger can also be used, it is generally not very
useful: simple bugs (uninitialized variables, memory management problems, etc.)
can be easily localized by control messages and the kernel-decoded oop message.

For more complex bugs, even a debugger can not help us too much unless the
operating system structure is very well understood. When debugging a kernel
module, there are a lot of unknowns in the equation: multiple contexts (we have
multiple processes and threads running at a time), interruptions, virtual
memory, etc.

You can use printk to display kernel messages to user space. It is similar to
printf's functionality; The only difference is that the transmitted message
can be prefixed with a string of "<n>", where n indicates the error level
(loglevel) and has values between 0 and 7. Instead of "<n>", the levels

Can also be coded by symbolic constants:

.. code-block:: c

    KERN_EMERG - n = 0
    KERN_ALERT - n = 1
    KERN_CRIT - n = 2
    KERN_ERR - n = 3
    KERN_WARNING - n = 4
    KERN_NOTICE - n = 5
    KERN_INFO - n = 6
    KERN_DEBUG - n = 7 


The definitions of all log levels are found in linux/kern_levels.h.
Basically, these log levels are used by the system to route messages sent to
various outputs: console, log files in /var/log etc.

To display printk messages in user space, the printk log level must be of
higher priority then ``console_loglevel`` variable. That is, the logging level is
less strict than the console_loglevel variable. For example, if the 
``console_loglevel`` has a value of 5 (specific to KERN_NOTICE), only messages
with loglevel stricter than 5 (i.e KERN_EMERG, KERN_ALERT, KERN_CRIT,
KERN_ERR, KERN_WARNING) will be shown.

Console-redirected messages can be useful for quickly viewing the effect of
executing the kernel code, but they are no longer so useful if the kernel
encounters an irreparable error and the system freezes. In this case, the logs
of the system must be consulted, as they keep the information between system
restarts. These are found in ``/var/log``  and are text files, populated with
syslogd and klogd during the kernel run. syslogd and klogd take the information
from the virtual file system mounted in /proc. In principle, with syslogd and
klogd turned on, all messages coming from the kernel will go to /var/log/kern.log.

A simpler version for debugging is using the /var/log/debug file. It is populated
only with the printk messages from the kernel with the KERN_DEBUG log level.

Given that a production kernel (similar to the one we're probably running with)
contains only release code, our module is among the few that send messages
prefixed with KERN_DEBUG . In this way, we can easily navigate through the
/var/log/debug information by finding the messages corresponding to a debugging
session for our module.

An example of use would be the following:

.. code-block:: bash

    # Clear the debug file of previous information (or possibly a backup)
    $ echo "New debug session" > /var/log/debug
    # Run the tests
    # If there is no critical error causing a panic kernel, check the output
    # if a critical error occurs and the machine only responds to a restart, 
      restart the system and check /var/log/debug.

The format of the messages must obviously contain all the information of 
interest in order to detect the error, but inserting in the code "printk" to 
provide detailed information can be as time-consuming as writing the code to 
solve the problem. This is usually a trade-off between the completeness of the 
debugging messages displayed using printk and the time it takes to insert these 
messages into the text.

A very simple way, less time-consuming for inserting printk and providing
the possibility to analyze the flow of instructions for tests is 
the use of the predefined constants __LINE__ , __LINE__ and __func__ :

    * ``__FILE__`` is replaced by the compiler with the name of the source file it is
      currently in the compilation.
    * ``__LINE__`` is replaced by the compiler with the line number on which the
      current instruction is found in the current source file.
    * ``__func__`` /``__FUNCTION__`` is replaced by the compiler with the name of the
      function in which the current instruction is found.

Note : ``__LINE__`` and ``__LINE__`` are part of ANSI C specification specifications:
``__func__`` is part of specification C99; ``__FUNCTION__`` is a GNU C
extension and is not portable; However, since we write code for the Linux kernel,
we can use it without any problems.

The following macrodefinition can be used in this case:

.. code-block:: c
   
   #define PRINT_DEBUG \ 
          printk (KERN_DEBUG "[% s]: FUNC:% s: LINE:% d \ n", __FILE__,
                  __FUNCTION__, __LINE__) 

Then, at each point where we want to see if it is "reached" in execution,
insert PRINT_DEBUG; This is a simple and quick way, and can yield by carefully 
analyzing the output.

The dmesg command is used to view the messages printed with printk but not
appearing on the console.

To delete all previous messages from a log file, run cat /dev/null > 
/var/log/debug. To delete messages displayed by the dmesg command, dmesg -c.


Dynamic debugging
-----------------

Dynamic ``dyndbg`` debugging enables dynamic debugging activation/deactivation.
Unlike printk, it offers more advanced printk options for the messages we want
to display - very useful for complex modules or troubleshooting subsystems.
This significantly reduces the amount of messages displayed, leaving only
those relevant for the debug context. To enable dyndbg, the kernel must be
compiled with the CONFIG_DYNAMIC_DEBUG option. Once configured, pr_debug(),
dev_dbg() and print_hex_dump_debug(), print_hex_dump_bytes() can be dynamically
enabled per call.

The ``/sys/kernel/debug/dynamic_debug/control`` file from the debugfs file debugfs
(where /sys/kernel/debug is the path to which debugfs were mounted) is used to
filter messages or view existing filters.

.. code-block:: c

   mount -t debugfs none /debug

Debugfs is a simple file system, used as a kernel-space interface and
user-space interface to configure different debug options. Any debug utility 
can create and use its own files / folders in debugfs.

For example, to display existing filters in dyndbg, you will use:

.. code-block:: bash
   
   cat /debug/dynamic_debug/control

And to enable the debug message from line 1603 in the svcsock.c file:

.. code-block:: bash

   echo 'file svcsock.c line 1603 +p' > /debug/dynamic_debug/control

The /debug/dynamic_debug/control file is not a regular file. Its display shows
the dyndbg settings on the filters. Writing in it with an echo will change
these settings (it will not actually make a write). Be aware that the file
contains settings for dyndbg debugging messages. Do not log in this file.

Dyndbg Options
~~~~~~~~~~~~~~

* ``func`` - just the debug messages from the functions that have the same
  name as the one defined in the filter.

  .. code-block:: bash

      echo 'func svc_tcp_accept +p' > /debug/dynamic_debug/control

* ``file`` - the name of the file(s) for which we want to display the debug
  messages. It can be just the source name, but also the absolute path or 
  kernel-tree path. 

  .. code-block:: bash

    file svcsock.c
    file kernel/freezer.c
    file /usr/src/packages/BUILD/sgi-enhancednfs-1.4/default/net/sunrpc/svcsock.c

* ``module`` - module name.

  .. code-block:: bash

     Modules sunrpc 

* ``format`` - only messages whose display format contains the specified string. 

   .. code-block:: bash

      format "nfsd: SETATTR" 

* line - the line or lines for which we want to enable debug calls.

  .. code-block:: bash

     # Triggers debug messages between lines 1603 and 1605 in the svcsock.c file
     $ echo 'file svcsock.c line 1603-1605 +p' > /sys/kernel/debug/dynamic_debug/control
     # Enables debug messages from the beginning of the file to line 1605
     $ echo 'file svcsock.c line -1605 +p' > /sys/kernel/debug/dynamic_debug/control

In addition to the above options, a series of flags can be specified that can
be added, removed, or set with operators +, - or = :

   * ``p`` activates the pr_debug() .
   * ``f`` includes the name of the function in the printed message.
   * ``I`` includes the line number in the printed message.
   * ``M`` includes the module name in the printed message.
   * ``T`` includes the thread id if it is not called from interrupt context
   * ``_`` no flag is set. 

Exercises
=========

.. important::

   .. include:: exercises-summary.hrst
   
   .. |LAB_NAME| replace:: kernel_modules
   

0. Intro
--------

Find the definitions of the following symbols in the Linux kernel:

* :c:func:`module_init` and :c:func:`module_exit`

  - what does the two macros do? What is ``init_module`` and ``cleanup_module``?
* :c:data:`ignore_loglevel`

  - What is this variable used for?

1. Module
---------

Generate the skeleton for the task named **1-2-test** then build and
copy the module to the VM. Perform the following tasks:

* load the kernel module.
  
* list the kernel modules and check if current module is present
    
* unload the kernel module
    
* view the messages displayed at loading/unloading the kernel module using
  ``dmesg`` command

.. note:: Read `Loading/unloading a kernel module`_ section. When unloading
          a kernel module, only the module name (without extension) can
          be specified.

2. Printk
---------

Watch the virtual machine console. Why were the messages not displayed directly
to the virtual machine console?

Inspect the source code file. Change the source code file so that messages are
displayed directly on the serial console.

.. hint:: Read the `Printk debugging`_ section of the lab and change
          the log level of the prints.

.. hint:: An alternative approach is to edit the boot options in
          ``tools/labs/qemu/Makefile``. Add ``ignore_loglevel`` option
	  to the qemu ``--append`` option.

.. hint:: Another option is to set the current log level by writting
          the desired log level to ``/proc/sys/kernel/printk``

3. Error
--------

Generate the skeleton for the task named **1-2-test**. Compile the
sources and get the corresponding kernel module. Why have compilation
errors occurred?

.. hint:: How does this module differ from the previous module?

Modify the module to solve the cause of those errors.

4. Sub-modules
--------------

Generate the skeleton for the task named **4-multi-mod**. Inspect the
C source files: ``mod1.c`` and ``mod2.c``. Module 2 contains only the
definition of a function used by module 1.

Create a Kbuild file that will lead to creating the ``multi_mod.ko``
from the two source files.

.. hint:: Read the `Compiling kernel modules`_ section of the lab.

Compile, copy, load and unload the kernel module. Make sure messages
are properly displayed on the console.

5. Kernel oops
--------------

Generate the skeleton for the task named *5-oops-mod** and inspect the
C source file. Notice where the problem will occur. Add -g to
compilation in the Kbuild file.

.. hint:: Read `Compiling kernel modules`_  section of the lab.

Compile the associated module and load it into the kernel. Identify the memory
address at which the oops appeared.

.. hint:: Read `Debugging`_ section of the lab.  To identify the
          address, follow the oops message and extract the value of
          the instructions pointer (EIP) register.

Determine which instruction has triggered the oops.

.. hint:: Use the /proc/modules information to get the load address of
          the kernel module.  Use, on the physical machine, objdump
          and/or addr2line . Objdump needs debugging support for
          compilation!  Read the lab's `objdump`_ and `addr2line`_
          sections.

Try to unload the kernel module. Notice that the operation does not
work because there are references from the kernel module within the
kernel since the oops; Until the release of those references (which is
almost impossible in the case of an oops), the module can not be
unloaded.

6. Module parameters
--------------------

Generate the skeletons for **6-cmd-mod** and inspect the C
``cmd_mod.c`` source file. Compile and copy the associated module and
load the kernel module to see the printk message. Then unload the
module from the kernel.

Without modifying the sources, load the kernel module so that the
message shown is ``Early bird gets tired``.

.. hint:: The str variable can be changed by passing a parameter to
          the module. Find more information `here
          <http://tldp.org/LDP/lkmpg/2.6/html/x323.html>`_.

7. Proc info
------------

Generate the skeleton for the task named **7-list-proc**. Add code to
display the Process ID (``PID``) and the executable name. The
information will be displayed both when loading and unloading the
module.

.. note::
          * In the Linux kernel, a process is described by the
	    :c:type:`struct task_struct`.  Use |LXR|_ to find the
	    definition of ``struct task_struct``.
	  
          * To find the structure field that contains the name of the
	    executable, look for the "executable" comment.

          * The pointer to the structure of the current process
	    running at a given time in the kernel is given by the
	    :c:macro:`current` variable (of the type ``struct task_struct
	    *``).

.. hint:: To use c:macro:`current` you'll need to include the header
          in which the ``struct task_struct`` is defined, i.e
          ``linux/sched.h``.

Compile copy and load the module. Unload the kernel module.

Repeat the loading/unloading operation. Note that the PIDs of the
displayed processes differ. This is because a module is being loaded
from the executable ``/sbin/insmod`` when the module is loaded and
when the module is unloaded a process is created from the executable
``/sbin/rmmod``.
