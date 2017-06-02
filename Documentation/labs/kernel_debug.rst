============
Kernel debug
============

.. slideconf::
   :theme: single-level
   :autoslides: false

.. slide:: Kernel Debugging
   :level: 1

.. slide:: Tools
   :level: 2

   * printk
   * dump_stack()
   * gdb
   * DEBUG_SLAB
   * kmemleak

.. slide:: dump_stack()
   :level: 2

   .. code-block:: shell

      CPU: 0 PID: 231 Comm: insmod Tainted: G 4.12.0-rc1+ #3
      Call Trace:
       dump_stack+0x5f/0x89
       crusher_simple_call+0x23/0x30 [crusher]
       crusher_init+0xa5/0xf0 [crusher]
       do_one_initcall+0x30/0x150
       ? do_init_module+0x17/0x1c6
       do_init_module+0x46/0x1c6
       load_module+0x1ea4/0x2290
       SyS_init_module+0xfd/0x120
       do_int80_syscall_32+0x5c/0x120
       entry_INT80_32+0x2f/0x2f
      EIP: 0x44902cc2
      EFLAGS: 00000206 CPU: 0
      EAX: ffffffda EBX: 0976e050 ECX: 00013250 EDX: 0976e008
      ESI: 00000000 EDI: bfb7b9c0 EBP: 00000000 ESP: bfb7b81c
       DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b

.. important::

   .. include:: exercises-summary.hrst
   .. |LAB_NAME| replace:: kernel_debug

Go to ``skels/kernel_debug/`` and inspect the file ``crusher.c``. We will use this module
to demonstrate some of the debugging tools available in Linux kernel.

dump_stack
==========

   1. Add a call to **dump_stack()** function inside **crusher_foo()**.
   2. Insert the crusher module as follows:

      .. code-block:: shell
        
         $ insmod skels/kernel_debug/crusher.ko test=0

   3. Notice the stack_trace. Who calls the **crusher_foo** function?

.. slide:: gdb
   :level: 2

   .. code-block:: shell
   
       BUG: unable to handle kernel NULL pointer dereference at   (null)
       IP: crusher_invalid_access+0x28/0x40 [crusher]
 
   .. code-block:: shell

      $ gdb ./skels/kernel_debug/crusher.ko
      $ (gdb) list *(crusher_invalid_access+0x28)
        0x78 is in crusher_invalid_access (crusher.c:35).

gdb
===

   1. Insert the crusher module as follows:

      .. code-block:: shell
        
         $ insmod skels/kernel_debug/crusher.ko test=1

   2. Inspect the stack trace, it should look like this:

      .. code-block:: shell
   
         BUG: unable to handle kernel NULL pointer dereference at   (null)
         IP: crusher_invalid_access+0x28/0x40 [crusher]
         *pde = 00000000 
         
         Oops: 0002 [#1] SMP
         Modules linked in: crusher(O+)
         CPU: 0 PID: 235 Comm: insmod Tainted: G           O    4.12.0-rc1+ #1
         Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS Ubuntu-1.8.2-1ubuntu1 04/01/2014
         task: c79864c0 task.stack: c71f6000
         EIP: crusher_invalid_access+0x28/0x40 [crusher]
         EFLAGS: 00000286 CPU: 0
         EAX: 00000000 EBX: 00000000 ECX: 00000000 EDX: c71a1c00
         ESI: c7173708 EDI: c88180f0 EBP: c71f7df0 ESP: c71f7dec
          DS: 007b ES: 007b FS: 00d8 GS: 0033 SS: 0068
         CR0: 80050033 CR2: 00000000 CR3: 0715f000 CR4: 00000690
         Call Trace:
          crusher_init+0x20/0x40 [crusher]
          do_one_initcall+0x30/0x150
          ? cache_alloc_debugcheck_after.isra.20+0x15f/0x2f0
          ? __might_sleep+0x35/0x80
          ? trace_hardirqs_on_caller+0x11c/0x1a0
          ? do_init_module+0x17/0x1c6
          ? kmem_cache_alloc+0xa0/0x1e0
          ? do_init_module+0x17/0x1c6
          do_init_module+0x46/0x1c6
          load_module+0x1ea4/0x2290
          SyS_init_module+0xfd/0x120
          do_int80_syscall_32+0x5c/0x120
          entry_INT80_32+0x2f/0x2f
         EIP: 0x44902cc2
         EFLAGS: 00000206 CPU: 0
         EAX: ffffffda EBX: 081f4050 ECX: 00012b4c EDX: 081f4008
         ESI: 00000000 EDI: bfef50b0 EBP: 00000000 ESP: bfef4f0c
          DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b
         EIP: crusher_invalid_access+0x28/0x40 [crusher] SS:ESP: 0068:c71f7dec
         CR2: 0000000000000000
         ---[ end trace 84b4aa2d1b642aea ]---

   3. For debugging, it is important to notice the address of ``EIP`` register. On host
      machine we will use **gdb** to debug the issue:

      .. code-block:: shell
        
         $ gdb ./skels/kernel_debug/crusher.ko
         $ (gdb) list *(crusher_invalid_access+0x28)
           0x78 is in crusher_invalid_access (crusher.c:35).

   4. Now that we know the line where the problem has occured, try to understand the issue
      and fix it!

.. slide:: DEBUG_SLAB
   :level: 2

   * red zoning
   * poisoning

   .. image:: slab.jpg

DEBUG_SLAB
==========

[insert poison slide]

   1. Insert the crusher module as follows:

      .. code-block:: shell
        
         $ insmod skels/kernel_debug/crusher.ko test=$NUMBER
   
      where NUMBER={2,3}.

   2. Check the Instruction Pointer (IP) and figure out what kind of error do we have for each
      value of NUMBER. Try to fix it!

.. slide:: kmemleak
   :level: 2

   * ``Documentation/kmemleak.txt``
   * objects are tracked similar to a garbage collector

   .. code-block:: shell
 
      $ echo clear > /sys/kernel/debug/kmemleak
      # test module
      $ echo scan > /sys/kernel/debug/kmemleak
      $ cat /sys/kernel/debug/kmemleak

kmemleak
========

   1. Enable kmemleak. Select DEBUG_KMEMLEAK symbol in .config file

     .. code-block:: shell

        $ make menuconfig
        # select DEBUG_KMEMLEAK=y
        $ make savedefconfig
        $ cp ./defconfig tools/labs/qemu/kernel_config.x86

   2. Use kmemleak to detect possible memory leaks:

      .. code-block:: shell
        
        # clear the list of all possible memory leaks
	$ echo clear > /sys/kernel/debug/kmemleak
        # insert crusher module
        $ insmod skels/kernel_debug/crusher.ko test=4
        # trigger an intermediate scan
        $ echo scan > /sys/kernel/debug/kmemleak
	# check for leaks
        $ cat /sys/kernel/debug/kmemleak

   3. Identify the problem and fix it. Verify again using kmemleak that the leak was fixed.

lockdep
=======

   1. Enable lockdep. Select CONFIG_LOCKDEP symbol in .config file

     .. code-block:: shell

        $ make menuconfig
        # select CONFIG_LOCKDEP=y
        $ make savedefconfig
        $ cp ./defconfig tools/labs/qemu/kernel_config.x86

   2. Insert the crusher module as follows:

      .. code-block:: shell
        
         $ insmod skels/kernel_debug/crusher.ko test=5

   3. Check the logs. If lockdep is enable you should see a warning in dmesg. Fix this issue.

