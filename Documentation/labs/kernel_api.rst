Kernel API
==========

Overview
--------

This labs introduces a set of ground concepts needed for starting Linux kernel
programming. If you are coming from a userspace background it is important to remember
that the Linux kernel is a self contained piece of code which doesn't have access to
your common userspace glibc functions (printf, malloc, free, read, write, memcpy). We will
see soon that the Linux kernel has its own API, with new functions for printing, memory
allocation and string manipulation.

Memory access
-------------

Because we are now closer to the hardware, there are some new rules regarding memory
access. Firstly, we can have several types of memory:

   * physical memory
   * virtual memory in the kernel address space
   * virtual memory in the process address space
   * resident memory, pages are always mapped into physical memory

Virtual memory from process address space it is not resident because of virtual memory mechanisms
implemented by the OS: pages might not be present in the physical memory because of demand paging.
In virtual memory from kernel address space, the code, data segment and stack are always resident
while the dinamically allocated memory can be swapped.

Working with resident memory the rules are simple: resident memory can be accessed at any time.
Anyhow, working with non-resident memory has some restrictions. Non-resident memory can only
be accessed from *process context*. Accessing non-resident memory from *interrupt context* can
have undefined behavior and for this reason the detecting such a situation the kernel will
reset or block the entire system.

Virtual memory of a process cannot be directly accessed from the kernel. The access must be
done using special functions which we learn about later.

One notable difference between userspace and kernel programming is the stack. In kernel
the stack size is fixed usually 4K. For this reasons we should avoid allocating large buffers
on stack or using recursive calls.

Execution contexts
------------------

When it comes about execution mode in kernel, there are two contexts:

   * **process**, when the code runs as a result of a system call or in the context of a kernel
     thread.
   * **interrupt**, when the code handles an interrupt or a deferrable action.

Depending on the implementation, some of the kernel functions could block the calling process.
For example, using a semaphore or waiting for an event. In this case, the current process is
put into waiting state and another process is scheduled. This is perfectly legal in context
process. But what happens if such a function is called in interrupt context? In this case, there
is no current process to save so the results are undefined.  By design, when the kernel detects
a situation like this it will hange the system.

Locking
-------

One of the important features of kernel is parallel processing. Linux kernel supports SMP and
preemptivity. This makes programming more difficult because we need to synchronize access to
global variables. There are two types of primitives used for synchronization:

  * busy waiting, here we have spinlocks which do not block the current context.
  * blocking, that will suspend the current process.

It is recommended to use blocking primitives but this is not always possible (for example in
interrupt context we can only use spinlocks).

Preemptivity
------------
Be aware, we should not mix the concept of preemptive multitasking with preemptive kernel.
Preemptive multitasking means that kernel can interrupt a process running in userspace and
schedule another process. A kernel is preemptive if a process running in kernel mode
(as a result of a system call) can be interrupted by another process.

Linux kernel API
----------------

Error codes convention
**********************

Linux kernel uses a similar coding convention with UNIX. 0 is returned for success and negative values
for errors. Here is a small example:


.. code-block:: c
   
   if (alloc_memory() != 0)
       return -ENOMEM;
    
   if (user_parameter_valid() != 0)
       return -EINVAL;
.. **

Complete list for error codes can be found in include/asm-generic/errno-base.h and
include/asm-generic/ernno.h

Strings
*******

Linux kernel has its own implemention of the usual character manipulation functions: strcpy,
strncpy, strlcpy, strcat, strncat, strlcat, strcmp, strncmp, strnicmp, strchr, strnchr,
strrchr, strstr, strlen, memset, memcpy, memmove, memscan, memcmp, memchr. This functions are declared
in include/linux/string.h and are implemented in lib/string.c

printk
******

This is the counterpart of userspace printf function and it is defined in include/linux/printk.h.
The first parameter of printk represents the log level, used to select which type of messages
one application should log.

.. code-block:: c
   
   #define KERN_EMERG   "<0>"  /* system is unusable */
   #define KERN_ALERT   "<1>"  /* action must be taken immediately */
   #define KERN_CRIT    "<2>"  /* critical conditions */
   #define KERN_ERR     "<3>"  /* error conditions */
   #define KERN_WARNING "<4>"  /* warning conditions */
   #define KERN_NOTICE  "<5>"  /* normal but significant condition */
   #define KERN_INFO    "<6>"  /* informational */
   #define KERN_DEBUG   "<7>"  /* debug-level messages */
   

.. **
So, at the simples level in order to log a warning message in kernel one should write:

.. code-block:: c

   printk(KERN_WARNING "My params %s %d\n", param1, param2);

If the loglevel is missing from printk call, the implicit loglevel will be used. Recommended
functions for logging are the one presented below:

.. code-block:: c

   pr_emerg(fmt, ...); /* echivalent cu printk(KERN_EMERG pr_fmt(fmt), ...); */
   pr_alert(fmt, ...); /* echivalent cu printk(KERN_ALERT pr_fmt(fmt), ...); */
   pr_crit(fmt, ...); /* echivalent cu printk(KERN_CRIT pr_fmt(fmt), ...); */
   pr_err(fmt, ...); /* echivalent cu printk(KERN_ERR pr_fmt(fmt), ...); */
   pr_warning(fmt, ...); /* echivalent cu printk(KERN_WARNING pr_fmt(fmt), ...); */
   pr_warn(fmt, ...); /* echivalent cu cu printk(KERN_WARNING pr_fmt(fmt), ...); */
   pr_notice(fmt, ...); /* echivalent cu printk(KERN_NOTICE pr_fmt(fmt), ...); */
   pr_info(fmt, ...); /* echivalent cu printk(KERN_INFO pr_fmt(fmt), ...); */
   

.. **

Memory allocation
*****************

The typical function to dinamically alocate memory in Linux kernel is kmalloc. Here is an example:

.. code-block:: c
   
   string = kmalloc (string_len + 1, GFP_KERNEL);
   if (!string) {
       //report error: -ENOMEM;
   }


.. **

The first parameter is the size of the requested memory allocation. The second parameter specifies
a mask for where to get the memory from. Most used values are:

  * GFP_KERNEL, using this could block the calling process. Shouln't be used in interrupt context.
  * GFP_ATOMIC, can be called in any context.

kmalloc returns a pointer to the allocated area or NULL in case of an error.

For freeing the memory allocated with kmalloc one must use kfree. This function has no restrictions
and cand be used in any context.

Linked lists
************

Because linked lists are heavily used, the Linux kernel API offers an uniform way of defining
and using them. This requires embedding an element of type :c:type:`struct list_head <list_head>` in
the linked data structure. Following code demonstrates linked lists usage in a well known structure
in linux kernel:

.. code-block:: c
   
   struct list_head {
       struct list_head *next, *prev;
   };
    
   struct task_struct {
       ...
       struct list_head children;
       ...
   };
.. **

Frequently used list functions are:

   * LIST_HEAD(name), declare the sentinel of a list
   * INIT_LIST_HEAD(struct list_head *head), used to initialize the sentinel for dinamically allocated lists
   * list_add(struct list_head *new, struct list_head *head) allocates a new element after head
   * list_del(struct list_head *entry), deletes the entry element from the list
   * list_entry(ptr, type, member), get the struct for this entry
   * list_for_each(pos, head) iterates over a list
   * list_for_each_safe(pos, n, head), iterate over a list safe against removal of list entry

The following code shows how to use the list API:

.. **

.. code-block:: c

   #include <linux/slab.h>
   #include <linux/list.h>
    
   struct pid_list {
       pid_t pid;
       struct list_head list;
   };
    
   LIST_HEAD(my_list);
    
   static int add_pid(pid_t pid)
   {
       struct pid_list *ple = kmalloc(sizeof *ple, GFP_KERNEL);
    
       if (!ple)
           return -ENOMEM;
    
       ple->pid = pid;
       list_add(&ple->list, &my_list);
    
       return 0;
   }
    
   static int del_pid(pid_t pid)
   {
       struct list_head *i, *tmp;
       struct pid_list *ple;
    
       list_for_each_safe(i, tmp, &my_list) {
           ple = list_entry(i, struct pid_list, list);
           if (ple->pid == pid) {
               list_del(i);
               kfree(ple);
               return 0;
           }
       }
    
       return -EINVAL;
   }
    
   static void destroy_list(void)
   {
       struct list_head *i, *n;
       struct pid_list *ple;
    
       list_for_each_safe(i, n, &my_list) {
           ple = list_entry(i, struct pid_list, list);
           list_del(i);
           kfree(ple);
       }
   }


All the operations with the lists are defined in include/linux/list.h.

.. **

Spinlocks
*********
spinlock_t (defined in linux/spinlock.h) is the base type which implements the spinlock mechanism
in Linux. The basic operations for the spinlock are:

   * spin_lock_init
   * spin_lock
   * spin_unlock.

Here is an example:

.. code-block:: c
   
   #include <linux/spinlock.h>
    
   DEFINE_SPINLOCK(lock1);
   spinlock_t lock2;
    
   spin_lock_init(&lock2);
    
   spin_lock(&lock1);
   /* critical region */
   spin_unlock(&lock1);
    
   spin_lock(&lock2);
   /* critical region */
   spin_unlock(&lock2);


.. **

Also Linux offers read/write spinlocks identified by rwlock_t type. The basic functions are:
   * rwlock_init
   * read_lock
   * write_lock.

Here is an example:

.. code-block:: c

   #include <linux/spinlock.h>
    
   DEFINE_RWLOCK(lock);
    
   struct pid_list {
       pid_t pid;
       struct list_head list;
   }; 
    
   int have_pid(struct list_head *lh, int pid)
   {
       struct list_head *i;
       void *elem;
    
       read_lock(&lock);
       list_for_each(i, lh) {
           struct pid_list *pl = list_entry(i, struct pid_list, list);
           if (pl->pid == pid) {
               read_unlock(&lock);
               return 1;
           }
       }
       read_unlock(&lock);
    
       return 0;
   }
    
   void add_pid(struct list_head *lh, struct pid_list *pl)
   {
       write_lock(&lock);
       list_add(&pl->list, lh);
       write_unlock(&lock);
   }

.. **

Mutex
*****

A mutex is represented by a variable of type struct mutex (defined in linux/mutex.h). The functions for
working with mutexes are:

.. code-block:: c

   #include <linux/mutex.h>
   
   /* functii pentru initializarea mutexului */
   void mutex_init(struct mutex *mutex);
   DEFINE_MUTEX(name);
   
   /* functii pentru achiziționarea mutexului */
   void mutex_lock(struct mutex *mutex);
   
   /* functie pentru eliberarea semaforului */
   void mutex_unlock(struct mutex *mutex);


Atomic variables
****************

Often we need to synchronize acess to a simple variable, for example a counter. For this we can
use a variable of type atomic_t (defined in include/linux/atomic.h) which can hold an integer.
Following code snippet presents some of the operations that can be done with an atomic_t:

.. code-block:: c
   
   #include <asm/atomic.h>
    
   void atomic_set(atomic_t *v, int i);
   int atomic_read(atomic_t *v);
   void atomic_add(int i, atomic_t *v);
   void atomic_sub(int i, atomic_t *v);
   void atomic_inc(atomic_t *v);
   void atomic_dec(atomic_t *v);
   int atomic_inc_and_test(atomic_t *v);
   int atomic_dec_and_test(atomic_t *v);
   int atomic_cmpxchg(atomic_t *v, int old, int new);

Here is an example of how to use atomic variables:

.. code-block:: c

   #define LOCKED		0
   #define UNLOCKED	1
    
   static atomic_t flag;
    
   static int my_acquire(void)
   {
   	int initial_flag;
    
   	/*
   	 * Check if flag is UNLOCKED; if not, lock it and do it atomically.
   	 *
   	 * This is the atomic equivalent of
   	 * 	if (flag == UNLOCKED)
   	 * 		flag = LOCKED;
   	 * 	else
   	 * 		return -EBUSY;
   	 */
   	initial_flag = atomic_cmpxchg(&flag, UNLOCKED, LOCKED);
   	if (initial_flag == LOCKED) {
   		printk(KERN_ALERT "Already locked.\n");
   		return -EBUSY;
   	}
    
   	/* Do your thing after getting the lock. */
   	[...]
   }
    
   static void my_release(void)
   {
   	/* Release flag; mark it as unlocked. */
   	atomic_set(&flag, UNLOCKED);
   }
    
   void my_init(void)
   {
   	[...]
   	/* Atomic variable is initially unlocked. */
   	atomic_set(&flag, UNLOCKED);
    
   	[...]
   }

.. **

We could also use an atomic variable for keeping track of the position inside a buffer. For example:

.. code-block:: c

   static unsigned char buffer[MAX_SIZE];
   static atomic_t size;
    
   static void add_to_buffer(unsigned char value)
   {
   	buffer[atomic_read(&size)] = value;
   	atomic_inc(&size);
   }
    
   static unsigned char remove_from_buffer(void)
   {
   	unsigned char value;
    
   	value = buffer[atomic_read(&size)];
   	atomic_dec(&size);
    
   	return value
   }
    
   static void reset_buffer(void)
   {
   	atomic_set(&size, 0);
   }
    
   void my_init(void)
   {
   	[...]
   	/* Initilized buffer and size. */
   	atomic_set(&size, 0);
   	memset(buffer, 0, sizeof(buffer));
    
   	[...]
   }

.. **

Bitwise atomic ops
******************

.. code-block:: c

   #include <asm/bitops.h>
    
   void set_bit(int nr, void *addr);
   void clear_bit(int nr, void *addr);
   void change_bit(int nr, void *addr);
   int test_and_set_bit(int nr, void *addr);
   int test_and_clear_bit(int nr, void *addr);
   int test_and_change_bit(int nr, void *addr);
   


