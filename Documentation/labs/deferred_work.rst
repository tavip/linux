=============
Deferred work
=============

Lab objectives
==============

* Understating deferred work (i.e. code scheduled to be executed at a
  later time)
* Implementation of common tasks that uses deferred work
* Understanding the peculiarities of synchronization for deferred work

    
Keywords: softirq, tasklet, struct tasklet_struct, bottom-half
handlers, jiffies, HZ, timer, struct timer_list, spin_lock_bh,
spin_unlock_bh, workqueue, struct work_struct, kernel thread, events/x

Background information
======================

Deferred work is a class of kernel facilities that allows one to
schedule code to be executed at a later timer. This scheduled code can
run either in the context process or in interruption context depending
on the type of deferred work. Deferred work is used to complement the
interrupt handler functionality since interrupts have important
requirements and limitations:

* The execution time of the interrupt handler must be as small as
  possible
* In interrupt context we can not use blocking calls

Using deferred work we can perform the minimum required work in the
interrupt handler and schedule an asynchronous action from the
interrupt handler to run at a later time and execute the rest of the
operations.
  
Deferred work that runs in interrupt context is also known as
bottom-half, since its purpose is to execute the rest of the actions
from an interrupt handler (top-half).
  
Timers are another type of deferred work that are used to schedule the
execution of future actions after a certain amount of time has passed.

Kernel threads are not themselves deferred work, but can be used to
complement the deferred work mechanisms. In general, kernel threads
are used as "workers" to process events whose execution contains
blocking calls.

There are three typical operations that are used with all types of
deferred work:

1. **Initialization**. Each type is described by a structure whose
   fields will have to be initialized. The handler to be scheduled is
   also set at this time.
2. **Scheduling**. Schedules the execution of the handler as soon as
   possible (or after expiry of a timeout).
3. **Masking** or **Canceling**. Disables the execution of the
   handler. This action can be either synchronous (which guarantees
   that the handler will not run after the completion of canceling) or
   asynchronous.


.. attention:: When doing deferred work cleanup, like freeing the
	       structures associated with the deferred work or
	       removing the module and thus the handler code from the
	       kernel, always use the synchronous type of canceling
	       the deferred work.


The main types of deferred work are kernel threads and softirqs. Work
queues are implemented on top of kernel threads and tasklets and
timers on top of softirqs. Bottom-half handlers was the first
implementation of deferred work in Linux, but in the meantime it was
replaced by softirqs. That is why some of the functions presented
contain *bh* in their name.

Softirqs
========

softirqs can not be used by device drivers, they are reserved for
various kernel subsystems. Because of this there is a fixed number of
softirqs defined at compile time. For the current kernel version we
have the following types defined:

.. code-block:: c

   enum {
       HI_SOFTIRQ = 0,
       TIMER_SOFTIRQ,
       NET_TX_SOFTIRQ,
       NET_RX_SOFTIRQ,
       BLOCK_SOFTIRQ,
       IRQ_POLL_SOFTIRQ,
       TASKLET_SOFTIRQ,
       SCHED_SOFTIRQ,
       HRTIMER_SOFTIRQ,
       RCU_SOFTIRQ,
       NR_SOFTIRQS
   };


Each type has a specific purpose:

* *HI_SOFTIRQ* and *TASKLET_SOFTIRQ* - running tasklets
* *TIMER_SOFTIRQ* - running timers
* *NET_TX_SOFIRQ* and *NET_RX_SOFTIRQ* - used by the networking subsystem
* *BLOCK_SOFTIRQ* - used by the IO subsystem
* *BLOCK_IOPOLL_SOFTIRQ* - used by the IO subsystem to increase performance when the iopoll handler is invoked;
* *SCHED_SOFTIRQ* - load balancing
* *HRTIMER_SOFTIRQ* - implementation of high precision timers
* *RCU_SOFTIRQ* - implementation of RCU type mechanisms [1]_

.. [1] RCU is a mechanism by which destructive operations
       (e.g. deleting an element from a chained list) are done in two
       steps: (1) removing references to deleted data and (2) freeing
       the memory of the element. The second setup is done only after
       we are sure nobody uses the element anymore. The advantage of
       this mechanism is that reading the data can be done without
       synchronization. For more information see
       Documentation/RCU/rcu.txt.

    
The highest priority is the *HI_SOFTIRQ* type softirqs, followed in
order by the other softirqs defined. *RCU_SOFTIRQ* has the lowest
priority.

Softirqs are running in interrupt context which means that they can
not call blocking functions. If the sofitrq handler requires calls to
such functions, work queues can be scheduled to execute these blocking
calls.

Tasklets
--------

A tasklet is a special form of deferred work that runs in interrupt
context, just like softirqs. The main between sofirqs are tasklets are
that tasklets can be allocated dynamically and thus they can be used
by device drivers. A tasklet is represented by :c:type:`struct
tasklet` and as many other kernel structures it needs to be
initialized before being used. A pre-initialized tasklet can defined
as following:

.. code-block:: c
		
   void handler(unsigned long data);
   
   DECLARE_TASKLET(tasklet, handler, data);
   DECLARE_TASKLET_DISABLED(tasklet, handler, data);

		
If we want to initialize the tasklet manually we can use the following
approach:

.. code-block:: c

   void handler(unsigned long data);

   struct tasklet_struct tasklet;

   tasklet_init(&tasklet, handler, data);

The *data* parameter will be sent to the handler when it is executed.

Programming tasklets for running is called scheduling. Tasklets are
running from softirqs. Tasklets scheduling is done with:

.. code-block:: c
		
   void tasklet_schedule(struct tasklet_struct *tasklet);

   void tasklet_hi_schedule(struct tasklet_struct *tasklet);
		
When using *tasklet_schedule*, a *TASKLET_SOFTIRQ* softirq is
scheduled and all tasklets scheduled are run. For
*tasklet_hi_schedule*, a *HI_SOFTIRQ* softirq is scheduled.

If a tasklet was scheduled multiple times and it did not run between
schedules, it will run once.  Once the tasklet has run, it can be
re-scheduled, and will run again at a later timer. Tasklets can be
re-scheduled from their handlers.

Tasklets can be masked and the following functions can be used:

.. code-block:: c

   void tasklet_enable(struct tasklet_struct * tasklet );
   void tasklet_disable(struct tasklet_struct * tasklet );

Remember that since tasklets are running from softirqs, blocking calls
can not be used in the handler function.

Timers
------

A particular type of deferred work, very often used, are timers. They
are defined by :c:type:`struct timer_list`. They run in interrupt
context and are implemented on top of softirqs.

To be used, a timer must first be initialized by calling :c:func:`setup_timer`:

.. code-block:: c

   #include <linux / sched.h>

   void setup_timer(struct timer_list * timer,
		    void (*function)(unsigned long),
		    unsigned long data);

The above function initializes the internal fields of the structure
and associates *function* as the timer handler; The data parameter
will be passed to the treatment function. Since timers are planned
over softirqs, blocking calls can not be used in the code associated
with the treatment function.

Scheduling a timer is done with :c:func:`mod_timer`:

.. code-block:: c

   int mod_timer(struct timer_list *timer, unsigned long expires);

Where *expires* is the time (in the future) to run the handler
function. The function can be used to schedule or reschedule a timer.

The time unit timers is *jiffie*. The absolute value of a jiffie
is dependent on the platform and it can be found using the
:c:type:`HZ` macro that defines the number of jiffies for 1 second. To
convert between jiffies (*jiffies_value*) and seconds (*seconds_value*),
the following formulas are used:

.. code-block:: c

   jiffies_value = seconds_value * HZ ;
   seconds_value = jiffies_value / HZ ;

The kernel mantains a counter that contains the number of jiffies
since the last boot, which can be accessed via the :c:type:`jiffies`
global variable or macro. We can use it to calculate a time in the
future for timers:

.. code-block:: c

   #include <linux/jiffies.h>

   unsigned long current_jiffies, next_jiffies;
   unsigned long seconds = 1;

   current_jiffies = jiffies;
   next_jiffies = jiffies + seconds * HZ;

To stop a timer, use :c:func:`del_timer` and :c:func:`del_timer_sync`:

.. code-block:: c

   int del_timer(struct timer_list *timer);
   int del_timer_sync(struct timer_list *timer);

Thse functions can be called for both a scheduled timer and an
unplanned timer. :c:func:`del_timer_sync` is used to eliminate the
races that can occur on multiprocessor systems, since at the end of
the call it is guaranteed that the timer processing function does not
run on any processor.

A frequent mistake in using timers is that we forget to turn off
timers. For example, before removing a module, we must stop the timers
because if a timer expires after the module is removed, the handler
function will no longer be loaded into the kernel and a kernel oops
will be generated.

The usual sequence used to initialize and schedule a one second
timeout is:

.. code-block:: c 

   #include <linux/sched.h>

   void timer_function(unsigned long arg);

   struct timer_list timer ;
   unsigned long seconds = 1;

   setup_timer(&timer, timer_function, 0);
   mod_timer(&timer, jiffies + seconds * HZ);
		
And to stop it:

.. code-block:: c   

   del_timer_sync(&timer);

Locking
-------

For synchronization between code running in process context (A) and
code running in softirq context (B) we need to use special locking
primitives. We must use spinlock operations augmented with
deactivation of bottom-half handlers on the current processor in (A),
and in (B) only basic spinlock operations. Using spinlocks makes sure
that we don't have races between multiple CPUs while deactivating the
softirqs makes sure that we don't deadlock in the softirq is scheduled
on the same CPU where we already acquired a spinlock.

We can use the :c:func:`local_bh_disable` and
:c:func:`local_bh_enable` to disable and enable softirqs handlers (and
since they run on top of softirqs also timers and tasklets):

.. code-block:: c
	
   void local_bh_disable(void);
   void local_bh_enable(void);

Nested calls are allowed, the actual reactivation of the softirqs is
done only when all local_bh_disable() calls have been complemented by
local_bh_enable() calls:

.. code-block:: c
		       
   /* We assume that softirqs are enabled */
   local_bh_disable();  /* Softirqs are now disabled */
   local_bh_disable();  /* Softirqs remain disabled */

   local_bh_enable();  /* Softirqs remain disabled */
   local_bh_enable();  /* Softirqs are now enabled */

.. attention:: These above calls will disable the softirqs only on the
   local processor and they are usually not safe to use, they must be
   complemented with spinlocks.


Most of the time device drivers will use special versions of spinlocks
calls for synchronization like :c:func:`spin_lock_bh` and 
:c:func:`spin_unlock_bh`:
   
.. code-block:: c

   void spin_lock_bh(spinlock_t *lock);
   void spin_unlock_bh(spinlock_t *lock);

Workqueues
----------   

Workqueues are used to schedule actions to run in process context. The
base unit with which they work is called work. There are two types of
work:

* :c:type:`structure work_struct` - it schedules a task to run at
  a later time
* :c:type:`struct delayed_work` - it schedules a task to run after at
  least a given time interval

A delayed work uses a timer to run after the specified time
interval. The calls with this type of work are similar to those for
:c:type:`struct work_struct`, but has **_delayed** in the functions
names.

Before using them a work item must be initialized. There are two types
of macros that can be used, one that declares and initializes the work
item at the same time and one that only initializes the work item (and
the declaration must be done separately):

.. code-block:: c
	       
   #include <linux/workqueue.h>

   DECLARE_WORK(name , void (*function)(struct work_struct *));
   DECLARE_DELAYED_WORK(name, void(*function)(struct work_struct *));

   INIT_WORK(struct work_struct *work, void(*function)(struct work_struct *));
   INIT_DELAYED_WORK(struct delayed_work *work, void(*function)(struct work_struct *));
	       
:c:func:`DECLARE_WORK` and :c:func:`DECLARE_DELAYED_WORK` declare and
initialize a work item, and :c:func:`INIT_WORK` and
:c:func:`INIT_DELAYED_WORK` initialize an already declared work item.

The following sequence declares and initiates a work item:

.. code-block:: c

   #include <linux/workqueue.h>

   void my_work_handler(struct work_struct *work);

   DECLARE_WORK(my_work, my_work_handler);
		
Or, if we want to initialize the work item separately:

.. code-block:: c

   void my_work_handler(struct work_struct * work);

   struct work_struct my_work;

   INIT_WORK(&my_work, my_work_handler);

Once declared and initialized, we can schedule the task using 
:c:func:`schedule_work` and :c:func:`schedule_delayed_work`:

.. code-block:: c

   schedule_work(struct work_struct *work);
   
   schedule_delayed_work(struct delayed_work *work, unsigned long delay);

:c:func:`schedule_delayed_work` can be used to plan a work item for
execution with a given delay. The delay time unit is jiffies.

Work items can not be masked by they can be canceled by calling
:c:func:`cancel_delayed_work_sync` or :c:func:`cancel_work_sync`:

.. code-block:: c

   int cancel_work_sync(struct delayed_work *work);
   int cancel_delayed_work_sync(struct delayed_work *work);

The call only stops the subsequent execution of the work item. If the
work item is already running at the time of the call, it will continue
to run. In any case, when these calls return, it is guaranteed that
the task will no longer run.

.. attention:: While there are versions of these functions that are
               not synchronous (.e.g. :c:func:`cancel_work`) do not
               use them when you are performing cleanup work otherwise
               race condition could occur.

We can wait for a workqueue to complete running all of its work items by calling :c:func:`flush_scheduled_work`:


.. code-block:: c

   void flush_scheduled_work(void);

This function is blocking and, therefore, can not be used in interrupt
context. The function will wait for all work items to be completed.
For delayed work items, :c:type:`cancel_delayed_work` must be called
before :c:func:`flush_scheduled_work`.

Finally, the following functions can be used to schedule work items on
a particular processor (:c:func:`schedule_delayed_work_on`), or on all
processors (:c:func:`schedule_on_each_cpu`):

.. code-block:: c

   int schedule_delayed_work_on(int cpu, struct delayed_work *work, unsigned long delay);
   int schedule_on_each_cpu(void(*function)(struct work_struct *));

A usual sequence to initialize and schedule a work item is the following:

.. code-block:: c

   void my_work_handler(struct work_struct *work);

   struct work_struct my_work;

   INIT_WORK(&my_work, my_work_handler);

   schedule_work(&my_work);
		
And for waiting for termination of a work item:

.. code-block:: c

   flush_scheduled_work();

As you can see, the *my_work_handler* function receives the task as
the parameter. To be able to access the module's private data, you can
use :c:func:`container_of`:

.. code-block:: c

   struct my_device_data {
       struct work_struct my_work;
       // ...
   };

   void my_work_handler(struct work_struct *work)
   {
      structure my_device_data * my_data;

      my_data = container_of(work, struct my_device_data,  my_work);
      // ...
   }
		
Scheduling work items with the functions above will run the handler in
the context of a thread kernel called *events/x*, where x is the
processor number. The kernel will initialize a kernel thread (or a
pool of workers) for each processor present in the system:

.. code-block:: shell

   $ ps -e
   PID TTY TIME CMD
   1?  00:00:00 init
   2 ?  00:00:00 ksoftirqd / 0
   3 ?  00:00:00 events / 0 <--- kernel thread that runs work items
   4 ?  00:00:00 khelper
   5 ?  00:00:00 kthread
   7?  00:00:00 kblockd / 0
   8?  00:00:00 kacpid
   
The above functions use a predefined workqueue (called events), and
they run in the context of the *events/x* thread, as noted
above. Although this is sufficient in most cases, it is a shared
resource and large delays in work items handlers can cause delays for
other queue users. For this reason there are functions for creating
additional queues.

A workqueue is represented by :c:type:`struct workqueue_struct`. A new
workqueue can be created with these functions:

.. code-block:: c

   struct workqueue_struct *create_workqueue(const char *name);
   struct workqueue_struct *create_singlethread_workqueue(const char *name);
		
:c:func:`create_workqueue` uses one thread for each processor in the
system, and :c:func:`create_singlethread_workqueue` uses a single
thread.

To add a task in the new queue, use :c:func:`queue_work` or
:c:func:`queue_delayed_work`:

.. code-block:: c

   int queue_work(struct workqueue_struct * queue, struct work_struct *work);
   
   int queue_delayed_work(struct workqueue_struct *queue,
                          struct delayed_work * work , unsigned long delay);

:c:func:`queue_delayed_work` can be used to plan a work for execution
with a given delay. The time unit for the delay is jiffies.

To wait for all work item to finish call :c:func:`flush_workqueue`:

.. code-block:: c
	
   void flush_workqueue(struct worksqueue_struct * queue);

And to destroy the workqueue call :c:func:`destroy_workqueue`

.. code-block:: c

   void destroy_workqueue(structure workqueque_struct *queue);

The next sequence declares and initializes an additional workqueue,
declares and initializes a work item and adds it to the queue:

.. code-block:: c

   void my_work_handler(struct work_struct *work);

   struct work_struct my_work;
   struct workqueue_struct * my_workqueue;

   my_workqueue = create_singlethread_workqueue("my_workqueue");
   INIT_WORK(&my_work, my_work_handler);

   queue_work(my_workqueue, &my_work);

And the next code sample shows how to remove the workqueue:

.. code-block:: c

   flush_workqueue(my_workqueue);
   destroy_workqueue(my_workqueue);
   
The work items planned with these functions will run in the context of
a new thread kernel called *my_workqueue*, the name passed to
:c:func:`create_singlethread_workqueue`.

Kernel threads
--------------   

Kernel threads have emerged from the need to run kernel code in
process context. Kernel threads are the basis of the workqueue
mechanism. Essentially, a thread kernel is a thread that only runs in
kernel mode and has no user address space or other user attributes.

To create a thread kernel, use :c:func:`kthread_create`:

.. code-block:: c

   #include <linux/kthread.h>

   structure task_struct *kthread_create(int (*threadfn)(void *data),
		                         void *data, const char namefmt[], ...);

* *threadfn* is a function that will be run by the kernel thread
* *data* is a parameter to be sent to the function
* *namefmt* represents the kernel thread name, as it is displayed in
  ps/top ; Can contain sequences %d , %s etc. Which will be replaced
  according to the standard printf syntax.

For example, the following call:

.. code-block:: c

   kthread_create (f, NULL, "%skthread%d", "my", 0);

Will create a thread kernel with the name mykthread0.

The kernel thread created with this function will stopped (in the
*TASK_INTERRUPTIBLE* state). To start the kernel thread, call the
:c:func:`wake_up_process`:

.. code-block:: c

   #include <linux/sched.h>

   int wake_up_process(struct task_struct *p);
   
Alternatively, you can use :c:func:`kthread_run` to create and run a
kernel thread:

.. code-block:: c   

   struct task_struct * kthread_run(int (*threadfn)(void *data)
		                    void *data, const char namefmt[], ...);

Even if the programming restrictions for the function running within
the kernel thread are more relaxed and scheduling is closer to
scheduling in userspace, there are, however, some limitations to be
taken into account. We will list below the actions that can or can not
be made from a thread kernel:

* can't access the user address space (even with copy_from_user,
  copy_to_user) because a thread kernel does not have a user address
  space
* can't implement busy wait code that runs for a long time; if the
  kernel is compiled without the preemptive option, that code will run
  without being preempted by other kernel threads or user processes
  thus hogging the system
* can call blocking operations
* can use spinlocks, but if the hold time of the lock is significant,
  it is recommended to use mutexes

The termination of a thread kernel is done voluntarily, within the
function running in the thread kernel, by calling :c:func:`do_exit`:

.. code-block:: c

   fastcall NORET_TYPE void do_exit(long code);

Most of the implementations of kernel threads handlers use the same
model and it is recommended to start using the same model to avoid
common mistakes:

.. code-block:: c

   #include <linux/kthread.h>

   DECLARE_WAIT_QUEUE_HEAD(wq);

   // list events to be processed by kernel thread
   structure list_head events_list;
   struct spin_lock events_lock;
   

   // structure describing the event to be processed
   struct event {
       struct list_head lh;
       bool stop;
       //...
   };

   struct event* get_next_event(void)
   {
       struct event *e;
   
       spin_lock(&events_lock);
       e = list_first_entry(&events_list, struct event*, lh);
       if (e)
           list_del(&events->lh);
       spin_unlock(&events_lock);
   }
   
   int my_thread_f(void *data)
   {
       struct event *e;

       while (true) {
           wait_event(wq, (e = get_next_event));

	   /* Event processing * /

	   if (e->stop)
	       break;
       }

       do_exit(0);
   }

   /* start and start kthread */
   kthread_run(my_thread_f, NULL, "%skthread%d", "my", 0);


With the template above, the kernel thread requests can be issued
with:

.. code-block:: c

   void send_cerere ( event structure * ev )
   {
       spin_lock(&events_lock);
       list_add(&ev->lh, &list_events);
       spin_unlock(events_lock);
       wake_up(&wq);
   }

Further reading
---------------   

* `Linux Device Drivers, 3rd ed., Ch. 7: Time, Delays, and Deferred Work <http://lwn.net/images/pdf/LDD3/ch07.pdf>`_
* `Scheduling Tasks <http://tldp.org/LDP/lkmpg/2.6/html/x1211.html>`_
* `Driver porting: the workqueue interface <http://lwn.net/Articles/23634/>`_
* `Workqueues get a rework <http://lwn.net/Articles/211279/>`_
* `Kernel threads made easy <http://lwn.net/Articles/65178/>`_
* `Unreliable Guide to Locking <http://www.kernel.org/pub/linux/kernel/people/rusty/kernel-locking/index.html>`_

