==============
Memory mapping
==============

Lab objectives
==============

* Understand address space mapping mechanisms
* Learn about the most important structures related to memory management

Keywords: address space, mmap, :c:type:`struct page`, :c:type:`struct vm_area_struct`, :c:type:`struct vm_struct`, :c:type:`remap_pfn_range`, :c:func:`SetPageReserved`, :c:func:`ClearPageReserved`

Background information
======================

In the Linux kernel it is possible to map a kernel address space to a
user address space. This eliminates the overhead of copying user-space
information into the kernel-space and vice versa. This can be done
through a device driver and the user-space device interface (/dev).

This feature can be used by implementing the mmap operation in the
device driver's :c:type:`struct file_operations` and using the mmap
system call in user-space.

Before discussing how we can the device driver map memory we will
first review a few important memory management concepts and
terminology.

The basic unit for virtual memory management is a page, which size is
usually 4K, but it can be up to 64K on same platforms. Whenever we
work with virtual memory we work with two types addresses: virtual
address and physical address. All CPU access (including from kernel
space) uses virtual addresses that are translated by the MMU into
physical address with the help of page tables.

A physical page of memory is identified by the Page Frame Number
(PFN). The PFN can be easily computed from the physical address by
dividing it with the size of the page (or by shifting the physical
address with PAGE_SHIFT bits to the right).

For efficiency reasons, the virtual address space is divided into
user-space and kernel-space. For the same reason, the kernel-space
contains memory mapped zone. called lowmem, which is contiguously
mapped in physical memory, starting from the lowest possible physical
address (usually 0). The virtual address where lowmem is mapped is
defined by :c:type:`PAGE_OFFSET`.

On 32bit system not all available memory can be mapped in lowmem and
because of that there is a separate zone in kernel-space called
highmem which can be used to arbitrarily map physical memory.

.. note:: Memory allocated by :c:func:`kmalloc` resides in lowmem and
          it is physically contiguous. Memory allocated with
          :c:func:`vmalloc` is not contiguous and does not reside in
          lowmem (it has a dedicated zone in kernel space).

Memory Structures
-----------------

Before discussing the mechanism of memory-mapping a device, we will
present some of the basic structures related to the memory management
subsystem of the Linux kernel.

:c:type:`struct page`
---------------------

:c:type:`struct page` is used to describe a memory physical page. The
kernel maintains a :c:type:`struct page` for all memory physical
pages in the system.

There are many functions that interact with this structure:

* :c:func:`virt_to_page` returns the page associated with a kernel
  virtual address
* :c:func:`pfn_to_page` returns the :c:type:`struct page` from the
  page frame number
* :c:func:`page_to_pfn` return the page frame number from a
  :c:type:`struct page`
* :c:func:`page_address` returns the virtual address of a
  :c:type:`struc page`; this functions can be called only for pages from
  lowmem
* :c:func:`kmap` creates a mapping in kernel for an arbitrary physical
  page (can be from highmem) and returns a virtual address that can be
  used to directly reference the page

:c:type:`struct vm_area_struct`
-------------------------------     

:c:type:`struct vm_area_struct` holds information about a contiguous
virtual memory area. The memory areas of a process can be viewed by
inspecting the *maps* attribute the process via procfs:

.. code-block:: c

   $ cat / proc / 1 / maps
   08048000-0804f000 r-xp 00000000 03:01 401624 / sbin / init
   0804f000-08050000 rw-p 00007000 03:01 401624 / sbin / init
   08050000-08071000 rw-p 08050000 00:00 0
   40000000 - 40016000 r-xp 00000000 03:01 369654 / lib / ld-2.3.2.so
   40016000 - 40017000 rw-p 00015000 03:01 369654 / lib / ld-2.3.2.so
   40017000 - 40018000 rw-p 40017000 00:00 0
   4001d000- 40147000 r-xp 00000000 03:01 371432 / lib / tls / libc-2.3.2.so
   40147000 - 40150000 rw-p 00129000 03:01 371432 / lib / tls / libc-2.3.2.so
   40150000 - 40153000 rw-p 40150000 00:00 0
   Bffff000-c0000000 rw-p bffff000 00:00 0
   Ffffe000-fffff000 --- p 00000000 00:00 0

A memory area is characterized by a start address, a stop address,
length, permissions.

A :c:type:`struct vm_area_struct` is created at each mmap call issued
from user-space. A driver that supports the mmap operation must
complete and initialize the associated :c:type:`struct
vm_area_struct`. The most important fields of this structure are:

* *vm_start*, *vm_end* - the beginning and end of the memory area
  respectively (these fields also appear in /proc/*/maps );
* vm_file - the pointer to the associated file structure (if any);
* vm_pgoff - the offset of the area within the file;
* vm_flags - a set of flags;
* vm_ops - a set of working functions for this area
* vm_next, vm_prev - the areas of the same process are chained by a
 list structure


:c:type:`struct mm_struct`
--------------------------    

:c:type:`struct mm_struct` encompasses all memory areas associated
with a process. The *mm* field of :c:type:`struct task_struct` is a
pointer to the :c:type:`struct mm_struct` of the current process.

Device driver memory mapping
============================

Memory mapping is one of the most interesting features of a Unix
system. From a driver's point of view, the memory-mapping facility
allows direct memory access to a user-space device.

To assign a mmap operation to a driver, the mmap field of the device
driver's :c:type:`struct file_operations` must be implemented. If that
is the case, the user-space process issues can then issue the *mmap*
system call of a file descriptor associated with the device.

The mmap system call takes the following parameters:

.. code-block:: c

   void *mmap(caddr_t addr, size_t len, int prot, int flags, int fd, off_t offset);

To map memory between a device and user-space, the user process must
open the device and issue the *mmap* system call with the resulting
file descriptor.

The device driver mmap operation has the following signature:

.. code-block:: c

   int (*mmap)(structure file *filp, struct vm_area_struct *vma);


The *filp* field is a pointer to a :c:type:`struct file` created when
the device is opened from user-space. The *vma* field is used to
indicate the virtual address space where the memory should be mapped
by the device. A driver should allocate memory (using
:c:func:`kmalloc`, :c:func:`vmalloc`, :c:func:`alloc_pages`) and then
map it to the user address space as indicated by the *vma* parameter
using helper functions such as :c:func:`remap_pfn_range`.

:c:func:`remap_pfn_range` will map a contiguous physical address space
into the virtual space represented by :c:type:`vm_area_struct`:

.. code-block:: c

   int remap_pfn_range (structure vm_area_struct * vma, unsigned long addr,
		        unsigned long pfn, unsigned long size, pgprot_t prot);

:c:func:`remap_pfn_range` expects the following parameters:

* *vma*  - the virtual memory space in which mapping is made;
* *addr* - the virtual address space from where remapping begins; page
  tables for the virtual address space between addr and addr + size
  will be formed as needed
* *pfn* the page frame number to which the virtual address should be
  mapped 
* size - the size (in bytes) of the memory to be mapped
* prot - protection flags for this mapping

Here is an example of using this function that contiguously maps the
physical memory starting at page frame number *pfn* (memory that was
previously allocated) to the *vma->vm_start* virtual address:

.. code-block:: c
		
   struct vm_area_struct * vma;
   unsigned long len = vma->vm_end - vma->vm_start;		
   int ret ;

   ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
   if (ret < 0) {
       pr_err("could not map the address area\n");
       return -EIO;
   }
		

To obtain the page frame number of the physical memory we must
consider how the memory allocation was performed. For each
:c:func`kmalloc`, :c:func:`vmalloc`, :c:func:`alloc_pages`, we must
used a different approach. For :c:func:`kmalloc` we can use something
like:

.. code-block:: c
   static char *kmalloc_area;

   unsigned long pfn = virt_to_phys((void *)kmalloc_area)>>PAGE_SHIFT;

while for :c:func:`vmalloc`:

.. code-block:: c

   static char *vmalloc_area;

   unsigned long pfn = vmalloc_to_pfn(vmalloc_area);

and finally for :c:func:`alloc_pages`:

.. code-block:: c

   struct page *page;

   unsigned long pfn = page_to_pfn(page);


.. attention:: Note that memory allocated with vmalloc is not
physically contiguous so if we want to map a vmalloc range we have to
map each page individually and compute the physical address for each
each page.

To avoid 

The PG_reserved bit must be enabled before a page is used. This bit
means that the page can not be swapped. Enabling is done using the
SetPageReserved macro . Macrodefinition receives as a parameter a
pointer to the page structure, struct page , which is obtained from
the kernel virtual address using the virt_to_page function, for
kmalloc assigned using kmalloc :

#define NPAGES 16

Static char * kmalloc_area ;
Int i ;

For ( i = 0 ; i < NPAGES * PAGE_SIZE ; i + = PAGE_SIZE ) {
SetPageReserved ( virt_to_page ( ( ( unsigned long ) kmalloc_area ) + i ) ) ;
}
And using the vmalloc_to_page function for addresses assigned using vmalloc :

#define NPAGES 16

Static char * vmalloc_area ;
Int i ;

For ( i = 0 ; i < NPAGES * PAGE_SIZE ; i + = PAGE_SIZE ) {
SetPageReserved ( vmalloc_to_page ( ( void * ) ( ( ( unsigned long ) vmalloc_area ) + i ) ) ) ;
}
Before the page is released ( vfree / vfree ), the reserved page bit must be disabled using the ClearPageReserved macro . It receives as parameter the same pointer to a page structure that was given to SetPageReserved .

Useful resources

Linux

Linux Device Drivers 3rd Edition - Chapter 15. Memory Mapping and DMA
Linux - mapping driver memory in user-space
Linux Device Driver mmap Skeleton
The Linux Kernel - Memory Management
Driver porting: supporting mmap ()
Device Drivers Concluded
Memory Mapped Files
mmap
