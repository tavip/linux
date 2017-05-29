=====================================================
Cross-compiling, Kernel, Uboot, Busybox, basic rootfs
=====================================================

Cross-compiling
===============

In order to compile for an embedded system a *cross-toolchain* (a set
of utilities including the compile like linker, assembler, compiler,
debugger, C library and other essential system libraries and
utilities) is needed. The cross-compiler term is sometimes used
instead of cross-toolchain.

A cross-tolchain is designed to run on another architecture than the
code it is producing. For example, we may have a toolchain that runs
on x86 and produces code for ARM.

Most of the distributions provide cross-compiler for the most popular
embedded architectures so there is no need to compile one from
scratch.

There are usually two types of cross-compilers available:

* a crude basic one that is mostly used to compile standalone programs
  like the kernel and bootloaders.

* a full fledged toolchain that includes support for dynamic linker,
  shared libraries, etc. This is used to compile regular programs that
  run in an OS environment.

The naming convention for a cross-compiler is: *compiler-name*-*architecture*-*os*-*abi*, where:

* *compiler-name* is the name of the compiler, e.g. gcc

* *architecture* is the name of the architecture, e.g. arm

* *os* is the name of the OS, e.g. linux, solaris

* *abi* is the name of the Application Binary Interface which defines
  the rules for calling conventions, shared library setup, etc. The most
  used are:

  * none - used for standalone programs like the kernel and bootloaders

  * gnu* - used for Linux systems

  * android* - used for Android systems

Emulation
=========

When developing for embedded system one has to copy the resulting
programs to the system itself to test it. This can be a cumbersome
process and sometimes we want to do quick checks without going through
this process.

In this case we can use emulation platforms like **qemu** to perform
simple verification tasks.

**qemu** can be run in two modes:

* a full system emulation mode, that is suitable to run full OSes,
  including bootloaders and the kernel (e.g. qemu-system-arm,
  qemu-systme-i386); this mode can take advantage of virtualization
  features if target and host are the same

* a userspace based emulation that is suitable to check programs built
  for an OS environment (e.g. qemu-arm-static, qemu-i386-status)

.. attention:: In order to use the userspace based emulation you
	       either need the full system library or to compile the
	       program statically.

qemu system emulation
---------------------

In full system emulation, qemu will emulate a full machine, including
disks, serial ports, networking devices, etc. Just like a physical
system, we need to have the bootloader and kernel compiled with the
right drivers that match the system components that qemu emulate so
that the system can succesfully boot.

You can configure the qemu machine by adding devices, tuning the
memory of the system, selecting a particular machine, selecting a
particular CPU, etc.

The most important parameters that needs to be passed to qemu are:

* *-kernel* expects a kernel binary image (zImage format)

* *-dtb* expects the Device Tree Blob (DTB) image; this is required on
  ARM systems

* *-machine* expects a machine name that is to be emulated; you can
  find out the list of supported emulated machines by using help as a
  machine name, .e.g.:

  .. code-block:: shell

     qemu-system-arm -machine help
     Supported machines are:
     akita                Sharp SL-C1000 (Akita) PDA (PXA270)
     borzoi               Sharp SL-C3100 (Borzoi) PDA (PXA270)
     ...
     versatileab          ARM Versatile/AB (ARM926EJ-S)
     versatilepb          ARM Versatile/PB (ARM926EJ-S)
     vexpress-a15         ARM Versatile Express for Cortex-A15
     vexpress-a9          ARM Versatile Express for Cortex-A9
     virt                 ARM Virtual Machine
     ...

* *-display* *backend* selects the display backend; none is used to
  suppress output, sdl is most common; note that not all machines have
  display output, in some cases you will need to use a serial device
  to connect to it

The most important parameters related to devices are:

* *-serial* *backend* add a serial port and redirect its output to a
  backend. There are multiple backends evailable: pty, chardev, file,
  stdio, pipe, udp, tcp, etc. See **man quemu-system-arm** for a
  description of each of these.

  To keep things similar as with connected to a physical embedded
  device in this tutorial we are going to use the pty backend.

  This creates a /dev/pts/... file that you can use to connect with
  minicom, picocom or other terminal programs, just like you would
  connect on serial ports (/dev/ttyS0, /dev/ttyUSB0).

  Note that the pts descriptors are allocated dynamically so you will
  have to check the quemu output to learn to which device to connect:

  .. code-block:: shell

     tools/labs$ qemu-system-arm ... -serial pty
     char device redirected to /dev/pts/12 (label serial0)

     minicom -D /dev/pts/12
     Welcome to minicom 2.7

     OPTIONS: I18n
     Compiled on Feb  7 2016, 13:37:27.
     Port /dev/pts/12, 14:56:31

     Press CTRL-A Z for help on special keys

     [    0.000000] Booting Linux on physical CPU 0x0
     [    0.000000] Linux version 4.10.9-yocto-standard (@opensuse132) (gcc version 6.3.7

* *-drive file=filename,media=media_type,if=if_type,id=driverid* adds
  a new drive and connects it to the block device which was the
  *drive=driveid* option.

  * *if_type* is the type of the interface where the drive is
    connected and can be on of: *ide*, *scsi*, *sd*, *mtd*, *floppy*,
    pflash, *virtio*.

  * *media_type* cab be *disk* or *cdrom*

* *-net nic,model=type,vlan=id* adds a new network card; use *-net
  nic,model=help* to see all available network cards that can be
  emulated; the vlad id is a number and identifies this network card;

* *-net tap,ifname=tap0,vlan=id,script=no,downscript=no* connects the
  network card identified by the vlan number to to a TAP iterfaces


Kernel configuration and build
==============================

Before building the kernel we need to configure it. Also, if we are
building a kernel with a cross-compiler we need to specify two things:

* the architecture for which we are building (use the ARCH parameter)

* the cross-compiler / cross-tool to user (use the CROSS_COMPILE
  parameter). Note that we just need to specify the prefix,
  e.g. arm-none-eabi-), the kernel will automatically append gcc, as,
  ld, etc. when it needs to use one of the toolchain programs

The Linux kernel has multiple options you can select when building an
image. It is recommended that you start from a know configuration, and
modify it to suit your needs.

To see what predefined configurations are available, use the help
target:

.. code-block:: shell

   $ make ARCH=arm help
   Cleaning targets:
   clean  - Remove most generated files but keep the config and
   enough build support to build external modules
   mrproper  - Remove all generated files + config + various backup files
   distclean  - mrproper + remove editor backup and patch files

   Configuration targets:
   config  - Update current config utilising a line-oriented program
   nconfig         - Update current config utilising a ncurses menu based
   program
   menuconfig  - Update current config utilising a menu based program
   ...
   acs5k_defconfig          - Build for acs5k
   acs5k_tiny_defconfig     - Build for acs5k_tiny
   am200epdkit_defconfig    - Build for am200epdkit
   aspeed_g4_defconfig      - Build for aspeed_g4
   aspeed_g5_defconfig      - Build for aspeed_g5
   ...

To select a particular configuration run the desired target:

.. code-block:: shell

   $ make ARCH=arm CROSS_COMPILE=arm-none-eabi- versatile_defconfig
   #
   # configuration written to .config
   #
   $

To update the configuration use the **menuconfig** target. You can use
the */* key to search for a particular config name.

.. note:: You will need to install the libncurses5-dev package to be
	  able to use the **menuconfig** target.

.. note:: Some config options depend on others and because of that may
	  not be enabled. Check the "Depends on" information and
	  enable other options as needed.

.. hint:: Use the (1), (2), shortcuts displayed in the search window
	  to jump to the config location and enable it. Just press the
	  shortcut number.

When configuring a certain options you might be able to compiled it in
the kernel image itself or as a module. A module can loaded in the
kernel at runtime and it helps keeping the kernel image small.

Once the config is complete build it by issuing make with no target
(but keep the ARCH and CROSS_COMPILE parameters). It is recommended to
use a parallel build (see the -j make option).

After the build you can find the kernel image in
*arch/arm/boot/zImage* (or arch/x86/boot/bzImage) and the dtb files in
*arch/arm/boot/dts*.


U-Boot
======

U-boot is an interactive bootloader which also supports limited
scripting but which enough for configuring the boot process.

U-boot's main role is to minimally initialize the hardware and load
the kernel image. To this end, it supports multiple peripheral devices
which can be used to load the kernel image from:

* various flash devices

* SD card / eMMC devices

* USB storage

* remotely, via TFTP; it supports various ethernet devices as well as
  standard USB ethernet dongles (USB CDC)

* raw block devices or FAT, EXT2 or EXT4 filesystems


Configuration and build
-----------------------

U-Boot shares the same build and configuration infrastructure as the
kernel. Because of that, building and configuring u-boot is very
similar with the kernel.

The same ARCH and CROSS_COMPILE make variables can be used to select
the architecture and cross-compiler. As the kernel, u-boot must be
first configure before building the image.

The supported images are stored in the *configs/* directory and you
can use one of the filenames there as the target to start with an
already predefined configuration.

To update a configuration use the **menuconfig** target. See the
`Kernel configuration and build` section on more information about the
**menuconfig** target.

Differently from the kernel, U-Boot does not suport modules, all
options are either built in the image or are disabled.

The compiled image is called *u-boot* and is placed in the top
directory.


Boot formats
------------

U-boot also support multiple "boot formats", basically it knows how to
interpret the given file, load it at the write address, descompress
it, performs relocations, and jump to the entry point. The supported
formats are:

* bootm - boot an application image from file

* bootz - boots a Linux zImage from memory

* bootelf - boot from an ELF image in memory

* bootefi - boots an EFI payload from memory

* bootp - boot image via network using BOOTP/TFTP protocol


Enviroment variables
--------------------

U-boot support persistent storage for its configuration, called
environment. The environment is composed of multiple variables, each
with its own values. The environment can be manipulated with the
following commands:

* env default [-f] -a - [forcibly] reset default environment

* env default [-f] var [...] - [forcibly] reset variable(s) to their default values

* env delete [-f] var [...] - [forcibly] delete variable(s)

* env export [-t | -b | -c] [-s size] addr [var ...] - export environment

* env import [-d] [-t [-r] | -b | -c] addr [size] - import environment

* env print [-a | name ...] - print environment

* env run var [...] - run commands in an environment variable

* env save - save environment

* env set [-f] name [arg ...]

Some environment variables have special meaning:

* bootcmd - the contents of this variable is run by u-boot if the boot
  is not interrupted

* bootargs - the contents of this variable is passed to the kernel

* bootdelay - how many seconds to wait before performing autoboot

* baudrate - the baudrate to be used by the serial driver

Other variables have a a somehow standard meaning, and they are used
in the boot scripts, although the actual names may be different
accross different boards:

* ftd_file - the DTB file name that is to be loaded by the boot scripts

* ftd_addr - address in memory where the DTB file is loaded

* kernel_addr / load_addr - address in memory where to load the kernel

Booting from eMMC
-----------------

Booting from eMMC involves two steps: selecting the eMMC device and
partition and than loading the zImage and DTB files from either the
raw device (from a given sector) or from a filesystem.

The usual sequence is the following: set the mmc device, issue the mmc
rescan commands to read the partitions, and then issuing the fatload,
ext2load or ext4load commands to load a file to a specified address.

.. code-block:: shell

   => mmc dev 0
   => mmc rescan
   => fatload mmc 0:1 ${loadaddr} ${zImage}
   => fatload mmc 0:1 ${ftdaddr} ${fdtfile}
   => bootz ${loadaddr} - ${ftdaddr}


Booting via TFTP
----------------

The TFTP boot process relies on DHCP to obtain an IP address from the
server and then load images (zImage, dtb) in memory with the TFTP
protocol. This is an example of a TFTP boot sequence:

.. code-block:: shell

   => tftpboot ${loadaddr} ${zImage}
   => tftpboot ${ftdaddr} ${fdtfile}
   => bootz ${loadaddr} - ${ftdaddr}

.. note:: If the an USB ethernet device is used you need to start the
	  usb stack first:

	  .. code-block:: shell

	     => usb start

Busybox
=======

Busybox is a standalone program that contains most of the basic tools
needed for a Linux system. It is used in small system, where we don't
have space to use the full system utilities. It alone can be used to
create a fully functional (although limited) Linux system.

.. note:: The tools that busybox offers are limited in functionality
	  as they are rewritten from scratch to save space.

Busybox uses the same configuration and building system as the kernel
or u-boot. As before the ARCH and CROSS_COMPILE make variables can be
used to select the architecture and cross-compiler.

The build process will produce a single binary, usually statically
linked, called *busybox*. In order to replicate the functionality of a
regular Linux system busybox creates symbolic links to itself, at
runtime, with the names of the commands it supports.

The commands the busybox supports are configurable from its
config. Here are the commands that are supported on a typical
configuration:

.. code-block:: shell

   BusyBox v1.22.1 (Ubuntu 1:1.22.0-15ubuntu1) multi-call binary.
   BusyBox is copyrighted by many authors between 1998-2012.
   Licensed under GPLv2. See source distribution for detailed
   copyright notices.

   Usage: busybox [function [arguments]...]
   or: busybox --list[-full]
   or: busybox --install [-s] [DIR]
   or: function [arguments]...

   BusyBox is a multi-call binary that combines many common Unix
   utilities into a single executable.  Most people will create a
   link to busybox for each function they wish to use and BusyBox
   will act like whatever it was invoked as.

   Currently defined functions:
   [, [[, acpid, adjtimex, ar, arp, arping, ash, awk, basename, blockdev,
   brctl, bunzip2, bzcat, bzip2, cal, cat, chgrp, chmod, chown, chpasswd,
   chroot, chvt, clear, cmp, cp, cpio, crond, crontab, cttyhack, cut, date,
   dc, dd, deallocvt, depmod, devmem, df, diff, dirname, dmesg, dnsdomainname,
   dos2unix, dpkg, dpkg-deb, du, dumpkmap, dumpleases, echo, ed, egrep, env,
   expand, expr, false, fdisk, fgrep, find, fold, free, freeramdisk, fstrim,
   ftpget, ftpput, getopt, getty, grep, groups, gunzip, gzip, halt, head,
   hexdump, hostid, hostname, httpd, hwclock, id, ifconfig, ifdown, ifup,
   init, insmod, ionice, ip, ipcalc, kill, killall, klogd, last, less, ln,
   loadfont, loadkmap, logger, login, logname, logread, losetup, ls, lsmod,
   lzcat, lzma, lzop, lzopcat, md5sum, mdev, microcom, mkdir, mkfifo, mknod,
   mkswap, mktemp, modinfo, modprobe, more, mount, mt, mv, nameif, nc,
   netstat, nslookup, od, openvt, passwd, patch, pidof, ping, ping6,
   pivot_root, poweroff, printf, ps, pwd, rdate, readlink, realpath, reboot,
   renice, reset, rev, rm, rmdir, rmmod, route, rpm, rpm2cpio, run-parts, sed,
   seq, setkeycodes, setsid, sh, sha1sum, sha256sum, sha512sum, sleep, sort,
   start-stop-daemon, stat, static-sh, strings, stty, su, sulogin, swapoff,
   swapon, switch_root, sync, sysctl, syslogd, tac, tail, tar, taskset, tee,
   telnet, telnetd, test, tftp, time, timeout, top, touch, tr, traceroute,
   traceroute6, true, tty, tunctl, udhcpc, udhcpd, umount, uname, uncompress,
   unexpand, uniq, unix2dos, unlzma, unlzop, unxz, unzip, uptime, usleep,
   uudecode, uuencode, vconfig, vi, watch, watchdog, wc, wget, which, who,
   whoami, xargs, xz, xzcat, yes, zcat

Busybox implements support for the init program, which is essential in
starting a Linux environment. It is much simpler that the full
implementation, but it still offers compatibility with the
*/etc/inittab* configuration file that is used to start services.

The inittab format is pretty simple: *<id>:<action>:<command to run>*,
where:

* *id* is an id for the services

* *action* is

  * *respawn* if we want the process restarted when it terminates

  * *sysinit* if this process should only run once when the system
    boots


Here is a very simple example that runs the */etc/rcS* script at boot
time, starts and respawn a shell on the serial console, and runs the
login utility on 5 the virtual terminals.

.. code-block:: c

   ::sysinit:/etc/rcS
   ttyAMA0::respawn:-/bin/sh
   ::respawn:/sbin/getty 38400 tty1
   ::respawn:/sbin/getty 38400 tty2
   ::respawn:/sbin/getty 38400 tty3
   ::respawn:/sbin/getty 38400 tty4
   ::respawn:/sbin/getty 38400 tty5


Linux root filesystem
======================

The root filesystem is the filesystem the kernel mounts before
executing the init process. Linux supports multiple types of root
filesystems: initrd (initial ram disk), NFS root (root filesystem over
the network) or a "regular" block filesystem.


initrd
------

initrd is used small embedded systems where we don't need a block
filesystem. In this case the bootloader loads a small cpio (.gz)
archive in memory and announce this to the kernel. The kernel will
decompress this archive in the special rootfs filesystem (which is a
ram based virtual filesystem).

.. note:: Most bootloaders have special boot commands or arguments to
	  boot commands that tells the kernel about where in RAM is
	  initrd located (e.g. bootz in u-boot, initrd in boot). In
	  case of qemu, you can specify the initrd with the *-initrd*
	  flag.

It is also used by distributions that address a large base of
different types of systems and where not all block devices or
filesystems are compiled in the kernel to keep the kernel images
small. In this case the distributions create an initrd that is
tailored to the system where the distribution is being installed which
contains the kernel modules needed to mount the root filesystem
(usually uncommon block device drivers, bus controller or
filesystems).

NFS root
--------

NFS root is the setup where the root filesystem is mounted over the
networking, from a Network File Server. This setup is activated when
the *nfsroot* option is passed to the kernel. It has the following
syntax:

.. code-block:: shell

   nfsroot=[<server-ip>:]<root-dir>[,<nfs-options>]

This setup also requires early IP configuration, before userspace
boots. This can be accomplished by setting another kernel options,
*ip*:

.. code-block:: shell

   ip=<client-ip>:<server-ip>:<gw-ip>:<netmask>:<hostname>:<device>:<autoconf>:<dns0-ip>:<dns1-ip>

The most common way to use this option is "ip=dhcp" (the *autoconf*
parameter can appear alone as the value to the *ip* parameter).

For details see about these options see
*Documentation/filesystems/nfs/nfsroot.txt*.


Exercises
=========

1. Installing cross-compiler
----------------------------

For this task we will install an ARM cross-compiler on an x86
system. Use the system package manager utility to install a gcc ARM
cross-compiler.

.. hint:: On Ubuntu you can use the *apt-cache* tool to query which
	  packages are available:

	  .. code-block:: shell

	     $ sudo apt-cache search gcc-arm
	     gcc-arm-linux-gnueabihf - GNU C compiler for the armhf architecture
	     gcc-arm-linux-androideabi - cross toolchain and binutils for Android/Bionic on ARM
	     gcc-arm-linux-gnueabi - GNU C compiler for the armel architecture
	     gcc-arm-none-eabi - GCC cross compiler for ARM Cortex-A/R/M processors
	     gcc-arm-none-eabi-source - GCC cross compiler for ARM Cortex-A/R/M processors (source)

.. hint:: On Ubuntu you can use the *apt-get* tool to install a
	  package:

	  .. code-block:: shell

	     $ sudo apt-get install gcc-arm-linux-gnueabi gcc-arm-none-eabi

For the next task compile a basic hello world program:

.. code-block:: c

   #include <stdio.h>

   int main(void)
   {
       printf("Hello world!\n");
   }


.. note:: Using se the arm-none-eabi-gcc cross-compiler will not be
	  successfull because this basic cross-compiler does not have
	  support for building regular programs that are suposed to
	  run in a OS.

	  .. code-block:: shell

	     tools/labs $ $ arm-none-eabi-gcc templates/kernel_uboot/hello_world.c
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-exit.o): In function `exit':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/stdlib/../../../../../newlib/libc/stdlib/exit.c:70: undefined reference to `_exit'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-sbrkr.o): In function `_sbrk_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/sbrkr.c:58: undefined reference to `_sbrk'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-writer.o): In function `_write_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/writer.c:58: undefined reference to `_write'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-closer.o): In function `_close_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/closer.c:53: undefined reference to `_close'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-lseekr.o): In function `_lseek_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/lseekr.c:58: undefined reference to `_lseek'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-readr.o): In function `_read_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/readr.c:58: undefined reference to `_read'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-fstatr.o): In function `_fstat_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/fstatr.c:62: undefined reference to `_fstat'
	     /usr/lib/gcc/arm-none-eabi/4.9.3/../../../arm-none-eabi/lib/libc.a(lib_a-isattyr.o): In function `_isatty_r':
	     /build/newlib-5zwpxE/newlib-2.2.0+git20150830.5a3d536/build/arm-none-eabi/newlib/libc/reent/../../../../../newlib/libc/reent/isattyr.c:58: undefined reference to `_isatty'
	     collect2: error: ld returned 1 exit status


After sucessfully compiling the program determine if this is indeed an
ARM binary.

.. hint:: Use the arm-linux-gnueabi-gcc cross-compiler and file to
	  inspect the binary.

	  .. code-block:: shell

	     tools/labs $ arm-linux-gnueabi-gcc templates/kernel_uboot/hello_world.c
	     tools/labs $ file a.out
	     a.out: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.3, for GNU/Linux 3.2.0, BuildID[sha1]=f673b3d4e773965cbfc954f1f95a3dd103093ea4, not stripped


Next, lets verify that this binary can be run on ARM platforms.

.. hint:: Use qemu in user emulation mode.

.. attention:: By default the program is compiled to use dynamic
	       shared objects and most likely you will not have an ARM
	       rootfs available, thus the emulation will fail:

	       .. code-block:: shell

		  tools/labs $ qemu-arm-static ./a.out
		  /lib/ld-linux.so.3: No such file or directory

.. hint:: Compile the program statically and then test it with qemu in
	  user emulation mode:

	  .. code-block:: shell

	     tools/labs$ arm-linux-gnueabi-gcc -static templates/kernel_uboot/hello_world.c
	     tools/labs$ qemu-arm-static ./a.out
	     Hello world!

2. Download and run Yocto precompiled images
---------------------------------------------

Download a Yocto qemu image for ARM and run it in full system
emulation.

.. note:: You can download precompiled Yocto images from
	  http://downloads.yoctoproject.org/releases/yocto.


Download both the prebuilt kernel image (*zImage*) and a DTB (Device
Tree Blob) for the emulated machine (zImage-versatile-pb.dtb).

Boot the kernel using the downloaded images.

.. hint:: Use the -kernel, -dtb and -machine parameters to
	  qemu-system-arm.

.. note:: You will notice that the kernel will start booting but it
	  will soon "panic" because there is no root filesystem to
	  mount:

	  .. code-block:: shell

			  [    7.199378] VFS: Unable to mount root fs on unknown-block(0,0)
			  [    7.201632] User configuration error - no valid root filesystem found
			  [    7.203310] Kernel panic - not syncing: Invalid configuration from end user prevg
			  [    7.206379] CPU: 0 PID: 1 Comm: swapper Not tainted 4.10.9-yocto-standard #1
			  [    7.208081] Hardware name: ARM-Versatile (Device Tree Support)
			  [    7.210825] [<c0017890>] (unwind_backtrace) from [<c0013c50>] (show_stack+0x20/0)
			  [    7.215067] [<c0013c50>] (show_stack) from [<c03be03c>] (dump_stack+0x20/0x28)
			  [    7.219920] [<c03be03c>] (dump_stack) from [<c00e90ac>] (panic+0xc4/0x240)
			  [    7.222418] [<c00e90ac>] (panic) from [<c097f400>] (mount_block_root+0x1d0/0x2b4)
			  [    7.227614] [<c097f400>] (mount_block_root) from [<c097f6ac>] (mount_root+0xd0/0)
			  [    7.232490] [<c097f6ac>] (mount_root) from [<c097f88c>] (prepare_namespace+0x188)
			  [    7.237579] [<c097f88c>] (prepare_namespace) from [<c097ef5c>] (kernel_init_free)
			  [    7.243256] [<c097ef5c>] (kernel_init_freeable) from [<c06d90cc>] (kernel_init+0)
			  [    7.249449] [<c06d90cc>] (kernel_init) from [<c000f890>] (ret_from_fork+0x14/0x2)
			  [    7.255485] ---[ end Kernel panic - not syncing: Invalid configuration from end g


Add a serial device to the qemu configuration that is redirected to a
pts virtual terminal and boot with no display. Connect to the serial
with minicom or picocom.

.. hint:: Find out the pts descriptor you need to connect to from the
	  qemu output. See `qemu system emulation`_ on how to add a
	  serial device.

Download qemu rootfs image and boot to userspace.

.. hint:: Download core-image-minimal-qemuarm.ext4 and add a new drive
	  to qemu. See `qemu system emulation`_ on how to add a new
	  drive.

.. hint:: If you are seeing the following error:

	  .. code-block:: shell

	     [    7.115236] 010f            4096 ram15
	     [    7.115245]  (driver?)
	     [    7.117659] 0800            8788 sda
	     [    7.117701]  driver: sd
	     [    7.120932] VFS: Unable to mount root fs on unknown-block(0,0)
	     [    7.122458] User configuration error - no valid root filesystem found

	  you need to tell the kernel what is the device that the root
	  filesystem neesd to be mounted


3. Configure and build kernel
-----------------------------

Configure your own kernel, build it and boot it. Make sure to change
the -kernel and -dtb options to point to the new files (see `Kernel
configuration and build`_).

.. hint:: Start with the default config for the veratile machine. Use
	  the ARCH and CROSS_COMPILE make variables to select the
	  architecture and the toolchain to use.

.. hint:: If the following error message is seen when booting the
	  compiled image:

	  .. code-block:: shell

	     010f            4096 ram15
	     (driver?)
	     Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)

	  and a drive option has been added to qemu, it means that the
	  disk driver is not configured. To determine which drivers we
	  need to compile, boot with the Yocto kernel and work your
	  way back from the /dev/sda device:

	  .. code-block:: shell

	     root@qemuarm:~# ls -la /sys/block/sda/device/  | grep driver
	     lrwxrwxrwx    1 root     root             0 May 27 15:37 driver -> ../../../../../../../../../bus/scsi/drivers/sd
	     root@qemuarm:~# ls -la /sys/block/ | grep sda
	     lrwxrwxrwx    1 root     root             0 May 27 14:43 sda -> ../devices/platform/amba/10001000.pci-controller/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0/block/sda
	     root@qemuarm:~# ls -la /sys/devices/platform/amba/10001000.pci-controller/pci0000:00/0000:00:0c.0/ | grep driver
	     lrwxrwxrwx    1 root     root             0 May 27 14:44 driver -> ../../../../../../bus/pci/drivers/sym53c8xx
	     -rw-r--r--    1 root     root          4096 May 27 14:44 driver_override
	     root@qemuarm:~# ls -la /sys/devices/platform/amba/10001000.pci-controller | grep driver
	     lrwxrwxrwx    1 root     root             0 May 27 14:47 driver -> ../../../../bus/platform/drivers/versatile-pci
	     -rw-r--r--    1 root     root          4096 May 27 14:47 driver_override

	  Based on the above information we determined that we need
	  the SCSI disk driver (BLK_DEV_SD), sym53c8xx SCSI driver as
	  well as the versatile-pci PCI driver. Search and enable
	  those two options (along with any required dependencies)
	  with the **menuconfig** target.

.. hint:: If the following error message is seen even after compiling
	  the disk drivers:

	  .. code-block:: c

	     [    4.366983] No filesystem could mount root, tried:
	     [    4.367000]  ext2
	     [    4.367097]  cramfs
	     [    4.367140]  minix
	     [    4.367183]  romfs
	     [    4.367232]
	     [    4.367416] Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-)

	  it means that we did not compile in the filesystem driver
	  for the image we are using. Enable to required filesystem
	  driver in the kernel config.


.. hint:: If the following error message is seen even after compling
	  the ext4 filesystem:

	  .. code-block:: c

	     [    5.908464] EXT4-fs (sda): re-mounted. Opts: data=ordered
	     [   13.364109] sd 0:0:0:0: [sda] Synchronizing SCSI cache
	     [   13.371596] reboot: System halted

	  just enabled devtmpfs.

Verify that you have booted your own compiled kernel.

.. hint:: Check the /proc/version file.


4. u-boot configuration and boot
--------------------------------

Download the uboot source from git://git.denx.de/u-boot.git and
checkout at v2017.05 tag. Then build an image for the
vexpress_ca9x4 machine and boot it with qemu.


.. hint:: For all make commands don't forget to set the architecture
	  and cross-compiler.

.. hint:: You can specify the uboot image to qemu with the -kernel
	  parameter.

.. hint:: Select the same machine to emulate as the image was built
	  for.

.. hint:: This particular uboot image will not show anything on the
	  display. Make sure to setup a serial device which you can
	  use to interact with uboot.

If all goes well you will be greeting with the following text on the
serial:

.. code-block:: c

   U-Boot 2017.05 (May 27 2017 - 20:45:08 +0300)

   DRAM:  128 MiB
   WARNING: Caches not enabled
   Flash: 128 MiB
   MMC:   MMC: 0
   *** Warning - bad CRC, using default environment

   In:    serial
   Out:   serial
   Err:   serial
   Net:   smc911x-0
   Hit any key to stop autoboot:  2

5. U-boot basic commands
-------------------------

Inspect the environment variables and determine the list and order of
the boot methods the bootloader will try, based on the curret /
default configuration.


.. hint:: Start inspecting from bootcmd, and follow the various
	  commands and variables it jumps through


6. U-boot: load and boot kernel from mmc
-----------------------------------------

For this task we want to prepare an SD card image based on the
downloaded Yocto image and use it to boot the system using u-boot.

Start by recompiling the kernel for the the *vexpress* platform and
verify that it boots using qemu.

.. note:: You won't be able to mount the root filesystem yet, since
	  this platform does not have suppport for PCI and SCSI, but
	  you should see the kernel boot and stuck here:

	  .. code-block:: shell

	  VFS: Cannot open root device "sda" or unknown-block(0,0): error -6
	  Please append a correct "root=" boot option; here are the available partitions:
	  1f00          131072 mtdblock0
	  (driver?)
	  1f01           32768 mtdblock1
	  (driver?)
	  Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
	  CPU: 0 PID: 1 Comm: swapper/0 Not tainted 4.12.0-rc1+ #18
	  Hardware name: ARM-Versatile Express
	  [<8010f2e0>] (unwind_backtrace) from [<8010b7e8>] (show_stack+0x10/0x14)
	  [<8010b7e8>] (show_stack) from [<803b1f10>] (dump_stack+0x94/0xa8)
	  [<803b1f10>] (dump_stack) from [<801d4d58>] (panic+0xdc/0x254)
	  [<801d4d58>] (panic) from [<8090123c>] (mount_block_root+0x1c0/0x294)
	  [<8090123c>] (mount_block_root) from [<8090142c>] (mount_root+0x11c/0x124)
	  [<8090142c>] (mount_root) from [<8090158c>] (prepare_namespace+0x158/0x1a0)
	  [<8090158c>] (prepare_namespace) from [<80900ed8>] (kernel_init_freeable+0x270/0x28)
	  [<80900ed8>] (kernel_init_freeable) from [<806547ac>] (kernel_init+0x8/0x110)
	  [<806547ac>] (kernel_init) from [<80107638>] (ret_from_fork+0x14/0x3c)
	  ---[ end Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block()


.. hint:: If you don't see any boot messages on the serial port use
	  the "console=ttyAMA0". The default config for this platform
	  is to use the frame buffer console.

Next configure qemu to emulate an SD card, change the kernel
parameters to use to use the SD card for the root filesystem and this
time we should be able to complete the boot to userspace.

.. hint:: Add a new drive to the qemu configuration with the interface
	  type set to *sd*.

.. hint:: If you see the following error:

	  .. code-block:: c

	     VFS: Cannot open root device "sda" or unknown-block(0,0): error -6
	     Please append a correct "root=" boot option; here are the available partitions:
	     1f00          131072 mtdblock0
	     (driver?)
	     1f01           32768 mtdblock1
	     (driver?)
	     b300          102400 mmcblk0
	     driver: mmcblk
	     Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)

	  it means that you didn't set a correct root option for the
	  kernel. Fortunatelly the output above is letting us know
	  what block device are available. Since we want to use the SD
	  card, mmcblk0 is our device.


OK, now that we determined the qemu configuration and kernel
parameters we can start to prepare the SD card so that we can use it
from uboot to load and boot the kernel.

First enlarge the Yocto image to make space for the zImage and dtb
file. For this we will use the **resize2fs** tool to increase the size
to 50M.

.. note:: You might need to run fsck.ext4 if the tools refuses to
	  increase the size.

Then copy the zImage and dtb file to the */boot* directory on the
image.

.. hint:: Mount the image with the -o loop option and copy the
	  required files, then umount the image:

	  .. code-block:: shell

	     sudo mount core-image-minimal-qemuarm.ext4 /tmp/mnt
	     sudo cp ~/src/linux/arch/arm/boot/zImage /mnt/tmp/boot
	     sudo cp ~/src/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dtb /tmp/mnt/boot/
	     sudo umount /tmp/mnt

At this point we are ready to use the SD card. Use qemu with the same
configuration as above and boot u-boot instead of the kernel.

.. hint:: Remove the -dtb option since we are going to load it from
	  the emulated SD card.

Now we should load the bzImage and dtb from the SD card to
memory. Before doing so, we need to inspect the memory mapping to
and determine where can we load them.

.. hint:: Use the bdinfo command:

   .. code-block:: shell

      => bdinfo
      arch_number = 0x000008E0
      boot_params = 0x60002000
      DRAM bank   = 0x00000000
      -> start    = 0x60000000
      -> size     = 0x08000000
      DRAM bank   = 0x00000001
      -> start    = 0x80000000
      -> size     = 0x00000004
      eth0name    = smc911x-0
      ethaddr     = 52:54:00:12:34:56
      current eth = smc911x-0
      ip_addr     = <NULL>
      baudrate    = 38400 bps
      TLB addr    = 0x67FF0000
      relocaddr   = 0x67F7D000
      reloc off   = 0x0777D000
      irq_sp      = 0x67EDCEF0
      sp start    = 0x67EDCEE0

   Notice that we can only use the first DRAM bank and that the memory
   start address is 0x60000000. Also note that boot_params are stored
   at 0x60002000, so we need to give it some room.

   Given the above, a good address to load the kernel image is
   0x60008000. As for the dtb, we need to take into account the kernel
   image size (and potential increases), so 0x61000000 seems a good
   address.

Once the load addreses are determined load the files into memory and
boot the kernel. Review the `Booting from eMMC`_  section.

.. hint:: Note that in our case the emulated SD card is not partition,
	  so avoid passing one to the ext4load commands.

7. U-boot: load and boot kernel from network
--------------------------------------------

For this task we want to load the kernel and DTB from the network. To
do so, we need to add a netwoking device to the qemu configuration and
allow it to communicate with the host. We also need to setup a DHCP
and TFTP server.

First we need to setup a tap interface. Use the
*tools/labs/qemu/create_net.sh* script for that.

.. note:: The script expects *tap0* or *tap1* as a parameter and it
	  will use it to setup either one. The script will also start
	  a DHCP and a TFTP server.

	  The TFTP server will serve files from the tftp directory
	  relative to the one you run the script, so create the
	  necessary link in that directory.

Next, use the tftpboot command to load the zImage and dtb file (see
`Booting via TFTP`_).


8. Minimal rootfs with busybox
-------------------------------

For this task we will create a minimum root filesystem from scratch
and boot it with qemu setup for the *vexpress* machine we prepared for
`6. U-boot: load and boot kernel from mmc`_. To keep things simple we
will not use u-boot, we will just boot directly the kernel image.

Lets start with compiling busybox. Download the busybox tree with:

.. code-block:: shell

   $ git clone git://busybox.net/busybox.git

Use the default config and built the busybox for the ARM, using the
cross-compiler.

.. hint:: If you run into the following error:

	  .. code-block:: shell

	     CC      applets/applets.o
	     In file included from include/libbb.h:13:0,
	     from include/busybox.h:8,
	     from applets/applets.c:9:
			  include/platform.h:157:23: fatal error: byteswap.h: No such file or directory
			  # include <byteswap.h>
			  ^
	     compilation terminated.

	  it probably means that the wrong cross-compiler was
	  used. Make sure the arm-linux-gnueabi- variant is used.

Verify that you can run the compiled busybux using the qemu in
userspace emulation mode.

.. hint:: If you run into this error:

	  .. code-block:: shell

	     tools/labs $ qemu-arm-static busybox
	     /lib/ld-linux.so.3: No such file or directory

	  you need to re-compile busybox statically. Use the
	  *menuconfig* target tu select this option.

Now that we have busybox compiled we can start creating a cpio archive
for the initrd image. Place busybox at the following path
*/bin/busybox* inside the cpio archive and try to boot from
initrd. Add "rdinit=/bin/busybox" to the kernel command line to select
which program the kernel will run (default is /init).

.. hint:: Create a temporary directory (e.g. *tmp*) and place busybox
	  at /tmp/bin/busybox then issue the following command to
	  create the initrd.cpio.gz archive:

	  .. code-block:: shell

	     cd tmp; find . | cpio -o -H newc > ../initrd.cpio; cd -
	     gzip -f initrd.cpio

.. note:: The boot will still crash, because just running /bin/busybox
	  will print the available commands and exit.

Next create a link /bin/sh to /bin/busybox in the cpio archive and try
to boot /bin/sh this time.

.. note:: You will notice that this time the system will boot to
	  userspace. However, there is not much we can do:

	  .. code-block:: shell

	     / # ls
	     /bin/sh: ls: not found
	     / # df -h
	     /bin/sh: df: not found


Use the --install -s option to create the links for all available
internal commands to /bin/busybox.

.. hint:: If you are seeing errors like:

	  .. code-block:: shell

	     busybox: /usr/bin/whois: No such file or directory
	     busybox: /usr/bin/xargs: No such file or directory

	  create the missing directories.

Notice that some programs like mount do not function properly because
important virtual filesystems are not mounted. Use the initrd template
from *tools/labs/templates/kernel_uboot/initrd/* to create a minimal
functional image.
