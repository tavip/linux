=====================================
Application development and debugging
=====================================

Application development
=======================

GCC
---

GCC is the default compiler suite on most Linux distributions.
We'll use a simple program that prints a string of characters to the standard
output to demonstrate gcc usage.

.. code-block:: c

    /* hello.c */
    #include <stdio.h>
     
    int main(void) 
    {
          printf("SO, ... hello world!\n");
     
          return 0;
    }

GCC uses the ``gcc`` command to compile C programs. A typical invocation is for
compiling a program from a single source file, in our case hello.c .

.. code-block:: bash

   so@spook$ ls
   hello.c
   so@spook$ gcc hello.c
   so@spook$ ls
   a.out  hello.c
   so@spook$ ./a.out
   SO, ... hello world!

.. code-block:: bash

   so@spook$ ls
   hello.c
   so@spook$ gcc hello.c -o hello
   so@spook$ ls
   hello  hello.c
   so@spook$ ./hello
   SO, ... hello world!


Therefore, the gcc hello.c command was used to compile the hello.c source file.
The result was the executable file a.out (default name used by gcc ). If you
want to get an executable with a different name, you can use the -o option.
Phases of compilation

Compilation refers to obtaining an executable file from a source file. As we
saw in the previous paragraph, the gcc command resulted in the ``hello`` executable
from the hello.c source file. Internally, gcc goes through several
processing phases of the source file until the executable is obtained. These
phases are highlighted in the diagram below: compilation phases
options

By default, gcc creates an executable from a source file. Using
various options, we can stop compiling at one of the intermediate phases as 
follows:


   * ``-E`` - only preprocess the source file
        gcc -E hello.c - will generate the preprocessed file that will default 
        to the standard output. 
   * ``-S`` - the compilation phase is performed
        gcc -S hello.c - will generate the file in assembly language hello.s 
   * ``-c`` - the assembly phase is carried out as well
        gcc -c hello.c - will generate the hello.o object file 

The above options can be combined with -o to specify the output file.
preprocessing

Compiling from multiple files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The examples so far deal with written programs in a single source file. In
reality, applications are complex and writing the entire code in one file makes it
hard to maintain and difficult to expand. In this regard, the application is
written in several source files called modules. A module typically contains functions
that play a common role.

The following files are used to support how to compile a program from multiple source files:

.. code-block:: c

    /* main.c */

    #include <stdio.h>
    #include "util.h"
     
    int main(void)
    {
         f1();
         f2();
         return 0;
    }

.. code-block:: c

      /* util.h */
    #ifndef UTIL_H
    #define UTIL_H   1
     
    void f1 (void);
    void f2 (void);
     
    #endif

.. code-block:: c

     /* f1. c */
    #include <stdio.h>
    #include "util.h"
     
    void f1(void)
    {
       printf("Current file name is %s\n", __FILE__);
     
    }

.. code-block:: c

      /* f2.c */
    #include <stdio.h>
    #include "util.h"
     
    void f1(void)
    {
       printf("Current file name is %s\n", __FILE__);
    }

In the above program, main function calls f1 and f2 to display various information.
To compile them all C files are sent as arguments to gcc :

.. code-block:: c

   so@spook$ ls
   f1.c  f2.c  main.c  util.h
   so@spook$ gcc -Wall main.c f1.c f2.c -o main
   so@spook$ ls
   f1.c  f2.c  main  main.c  util.h
   so@spook$ ./main 
   Current file name f1.c
   Current line 8 in file f2.c

The executable was called main ; For this, we used the -o option.

Note the use of the util.h header to declare f1 and f2. Declaring a function is
done by specifying the header. The header file is included in the main.c
file so that it knows the call format of the f1 and f2 functions. The functions
f1 and f2 are defined, respectively, in f1.c and f2.c Their code is integrated 
into the executable at the time of the link-editing.

Generally, to get a multiple-source executable, it's customary to compile each source
to the object mode and then link-editing them:

.. code-block:: c

   so@spook$ ls
   f1.c  f2.c  main.c  util.h
   so@spook$ gcc -Wall -c  f1.c
   so@spook$ gcc -Wall -c  f2.c
   so@spook$ gcc -Wall -c  main.c
   so@spook$ ls
   f1.c  f1.o  f2.c  f2.o  main.c  main.o  util.h
   so@spook$ gcc -o main main.o f1.o f2.o
   so@spook$ ls
   f1.c  f1.o  f2.c  f2.o  main  main.c  main.o  util.h
   so@spook$ ./main 
   Current file name f1.c
   Current line 8 in file f2.c

Note that the executable main is obtained by linking the object modules. This
approach has the advantage of efficiency. If the source file f2.c changes, then
it will only need to be compiled and re-edited. If a direct executable had been
obtained from sources then all three files would be compiled and then re-edited
the link-editing. The time consumed would be much higher, especially during the
development phase when the compilation phases are frequent, and only modified
source files are being compiled.

Decreasing development time by compiling only the sources that have been
modified is the basic motivation for the existence of automation tools such as
make.


GNU Make
--------

Make is a utility that allows automation and efficiency of tasks. It is 
especially used to automate program compilation. As has been said, it is 
inefficient to compile each file of each file and then link-editing to get an 
executable from multiple sources. Each file is compiled separately, and only 
one modified file will be recompiled.
Simple example of Makefile

The make utility uses a configuration file called ``Makefile``. Such a file
contains rules and automation commands.

.. code-block:: bash

    #Makefile

    all:
           gcc -Wall hello.c -o hello
    clean:
           rm -f hello

.. code-block:: bash

   so@spook$ make
   gcc -Wall hello.c -o hello
   so@spook$ ./hello
   SO, ... hello world!

.. code-block:: bash

   so@spook$ make clean
   rm -f hello
   so@spook$ make all
   gcc -Wall hello.c -o hello

The example above contains two rules: all and clean. When executing the make
command, the first rule in Makefile is executed (in this case all, no matter
the name). The executed command is ``gcc -Wall hello.c -o hello``. You can
explicitly specify which rule to execute by submitting as a make command.
(make clean to delete the hello executable and make all to get that executable 
again).

By default, GNU Make searches the GNUmakefile, Makefile, makefile files and 
analyzes them in order. To specify which Makefile file to analyze, use the -f 
option. Thus, in the example below, we use the Makefile.ex1 file:

The following is a syntax of a rule from a Makefile file:

    * ``target`` - is usually the file that will be obtained by running the command 
      command. As noted in the previous example, it may be a virtual target that does 
      not have a file associated with it.
    * prerequisites - represent the dependencies required to follow the rule; 
     Usually are files required to achieve the target.
    * ``<Tab>`` - represents the tab character and must be used before the order is 
     specified.
    * ``command`` - a list of commands (none, one, any) run when the target is 
      reached. 

An example for a Makefile file is:

.. code:: block

    # Makefile.ex2

    all: hello
     
    hello: hello.o
            gcc hello.o -o hello
     
    hello.o: hello.c
            gcc -Wall -c hello.c
     
    clean:
            rm -f *.o *~ hello

Debugging
=========

strace
------

``strace`` intercepts and records system calls made by a process and the signals it
receives. In the simplest form strace runs the specified command until the
associated process ends.

.. code-block:: bash

   $strace cat /proc/cpuinfo
   execve("/bin/cat", ["cat", "/proc/cpuinfo"], [/* 30 vars */]) = 0
   open("/proc/cpuinfo", O_RDONLY)         = 3
   read(3, "processor\t: 0\nvendor_id\t: Genuin"..., 32768) = 3652
   write(1, "processor\t: 0$\nvendor_id\t: Genui"..., 7512) = 7512

The most common options for strace are:

   * ``-f``, this option will be followed and child processes created by the
     current process
   * ``-o filename``, by default strace displays the information to the stderr,
     with this option, the output will be put in the filename file
   * ``-p pid``, the pid of the tracking process.
   * ``-e expression``, changes the calls you are looking for.

.. code-block:: bash

   daniel@debian$ strace -f -e connect,socket,bind -p $(pidof iceweasel)
   Process 6429 attached with 30 threads - interrupt to quit
   socket(PF_INET, SOCK_STREAM, IPPROTO_IP) = 50
   connect(50, {sa_family=AF_INET, sin_port=htons(80), sin_addr=inet_addr("141.85.227.65")}, 16)= -1 EINPROGRESS

Another utility related to strace is ``ltrace``. It tracks library calls

gdb
---

The purpose of a debugger (for example, GDB) is to allow us to inspect what 
happens inside a program while it is running or when a fatal error occurred.

``GDB`` can be used in two ways to debug the program:

   * Running it using the gdb command
   * Using the core file generated by a serious error (usually a segmentation
     fault) 

The second is useful if the bug was not corrected before launching the program.
In this case, if the user encounters a serious error, he can send the
programmer the core file with which he can debug the program and correct the
bug.

The simplest form of debugging with GDB is where we want to determine the
program line where the error occurred. For example, we consider the following 
program:

.. code-block:: c

    #include <stdio.h>
     
    int f(int a, int b)
    {
    	int c;    
    	c = a + b;    
    	return c;
    }
     
    int main()
    {
    	char *bug = 0;
    	*bug = f(1, 2);
    	return 0;
    }

After compiling the program, it can be debugged using GDB. After starting the
troubleshooting program, GDB enters interactive mode. The user can then use commands
to debug the program:

.. code-block:: bash

   $ gcc -Wall -g add.c
   $ gdb a.out
   [...]
   (gdb) run
   Starting program: a.out

   Program received signal SIGSEGV, Segmentation fault.
   0x08048411 in main () at add.c:13
   13              *bug=f(1, 2);
   (gdb)

The first command you use is run . This command will start running the program. 
If this command receives arguments from the user, they will be sent to the 
program. Before going to the basic commands in gdb, let's demonstrate how to 
troubleshoot a program using the core file:

.. code-block:: bash

   $ gcc -Wall -g add.c
   $ gdb a.out
   [...]
   (gdb) run
   Starting program: a.out
    
   Program received signal SIGSEGV, Segmentation fault.
   0x08048411 in main () at add.c:13
   13              *bug=f(1, 2);
   (gdb)

Basic GDB commands
~~~~~~~~~~~~~~~~~~

Some of the basic commands in gdb are:

   * ``b[reakpoint]`` - Receives as argument a function name (ex: main), a line
     number, and possibly a file (eg: ``break source.c: 50``), a function
     (``b source.c: my_function``) or an  address (ex: ``breakpoint *0x80483d3).
   * ``n[ext]`` - will continue executing the program until the next line in the
     source code is reached. If the line to execute contains a function call, the 

     function will be executed completely.
   * ``s[tep] - if you want to inspect the functions.
   * ``fin[ish]`` - if you want to exit the current function. 

The use of these commands is exemplified below:

.. code-block:: bash
   
   $ gdb a.out
   (gdb) break main
   Breakpoint 1 at 0x80483f6: file add.c, line 12.
   (gdb) run
   Starting program: a.out
    
   Breakpoint 1, main () at add.c:12
   12              char *bug=0;
   (gdb) next
   13              *bug=f(1, 2);
   (gdb) next
    
   Program received signal SIGSEGV, Segmentation fault.
   0x08048411 in main () at add.c:13
   13              *bug=f(1, 2);
   (gdb) run
   The program being debugged has been started already.
   Start it from the beginning? (y or n) y
   Starting program: a.out
   Breakpoint 1, main () at add.c:12
   12              char *bug=0;
   (gdb) next
   13              *bug=f(1, 2);
   (gdb) step
   f (a=1, b=2) at add.c:8
   6               c=a+b;
   (gdb) next
   7               return c;
   (gdb) next
   8      }
   (gdb) next
    
   Program received signal SIGSEGV, Segmentation fault.
   0x08048411 in main () at add.c:13
   13              *bug=f(1, 2);
   (gdb)
   
    * ``list`` - this command will list the source file of the debug program. The 
      command receives as argument a line number (possibly a file name), a function 
      or an address from which to list. The second argument is optional and specifies 
      how many lines will be displayed. If the command has no parameter, it will list 
      where the last view stopped.
    * ``continue`` - is used when continuing to run the program.

.. code-block:: bash
   
   $ gdb a.out
   (gdb) list add.c:1
   1       #include <stdio.h>
   2
   3       int f(int a, int b)
   4       {
   5               int c;
   6               c=a+b;
   7               return c;
   8       }
   (gdb) break add.c:6
   Breakpoint 1 at 0x80483d6: file add.c, line 6.
   (gdb) run
   Starting program: a.out
    
   Breakpoint 1, f (a=1, b=2) at add.c:6
   6               c=a+b;
   (gdb) next
   7               return c;
   (gdb) continue
   Continuing.
    
   Program received signal SIGSEGV, Segmentation fault.
   0x08048411 in main () at add.c:13
   13              *bug=f(1, 2);

   * ``print`` - it can display the values of the variables from the current
     function or the global variables. print can get as complex argument and
     expressions (pointers deferentiators, variables referencers, arithmetic
     expressions, almost any valid C expression). In addition, print can display
     data structures such as struct and union or evaluate functions and return their
     result.

.. code-block:: bash
   
   $ gdb a.out
   (gdb) break f
   Breakpoint 1 at 0x80483d6: file add.c, line 6.
   (gdb) run
   Starting program: a.out
    
   Breakpoint 1, f (a=1, b=2) at add.c:6
   6               c=a+b;
   (gdb) print a
   $1 = 1
   (gdb) print b
   $2 = 2
   (gdb) print f(a, b)
   $3 = 3
   (gdb) print c
   $4 = 1073792080
   (gdb) next
   7               return c;
   (gdb) print c
   $5 = 3
   (gdb) finish
   Run till exit from #0  f (a=1, b=2) at add.c:7
   0x08048409 in main () at add.c:13
   13              *bug=f(1, 2);
   Value returned is $5 = 3
   (gdb) print bug
   $6 = 0x0
   (gdb) print (struct sigaction)bug
   $13 = {__sigaction_handler =
    {
       sa_handler = 0x8049590 <object.2>,
       sa_sigaction = 0x8049590 <object.2>
    },
    sa_mask =
    {
      __val =
     {
       3221223384, 1073992320, 1, 3221223428,
       3221223436, 134513290, 134513760, 0, 3221223384,
       1073992298, 0, 3221223436, 1075157952,
       1073827112, 1, 134513360, 0, 134513393, 134513648, 1,
       3221223428, 134513268, 134513760, 1073794080,
       3221223420, 1073828556, 1, 3221223760, 0,
       3221223804, 3221223846,	3221223866
     }
    },
    sa_flags = -1073743402,
    sa_restorer = 0xbffff9f2}
   (gdb)


Working with memory
===================

Working with heap is one of the main causes of programming problems. Working
with pointers, the need to use system/library calls for assignment/assignment
can lead to a number of issues that affect (often fatal) the operation of a
program.

The most common problems with memory are:

  * invalid access to memory - which prevents access to areas that have not
    been allocated or have been released.
  * memory leaks - situations where the reference to a previously assigned area
    is lost. That area will remain busy until the process ends. 

Both issues and utilities that can be used to combat them will be presented 
below.

mcheck - check the heap consistency
-----------------------------------

``glibc`` allows you to check the consistency of the heap by calling mcheck defined
in mcheck.h . The mcheck call forces malloc to perform various consistency
checks such as writing over a block assigned to malloc .

Alternatively, you can use the -lmcheck option to link the program without
affecting its source.

The simplest option is to use the MALLOC_CHECK_ environment MALLOC_CHECK. If
a program will be executed with the configured MALLOC_CHECK_ variable, then
error messages will be displayed (eventually the program will be aborted).

The following is an example of a code with problems in allocating and using the 
heap:

.. code-block:: c

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
     
    int main(void)
    {
        int *v1;
     
        v1 = malloc(5 * sizeof(*v1));
        if (NULL == v1) {
                perror("malloc");
                exit (EXIT_FAILURE);
        }
     
        /* overflow */
        v1[6] = 100;
     
        free(v1);
     
        /* write after free */
        v1[6] = 100;
     
        /* reallocate v1 */
        v1 = malloc(10 * sizeof(int));
        if (NULL == v1) {
                perror("malloc");
                exit (EXIT_FAILURE);
        }
     
        return 0;
    }

Below you can see how the program is compiled and run. First, it runs without 
mcheck options, and then defines the MALLOC_CHECK_ environment variable when 
running the program. It is noted that although the space allocated for vector 
v1 is exceeded and the vector is referenced after the space is released, a 
simple run does not result in the display of any error.

However, if we define the MALLOC_CHECK_ environment MALLOC_CHECK_ , the two 
errors are detected. Note that an error is detected only at the time of a new 
memory call intercepted by mcheck.

.. code-block:: bash

   so@spook$ make
   cc -Wall -g    mcheck_test.c   -o mcheck_test
   so@spook$ ./mcheck_test  
   so@spook$ MALLOC_CHECK_=1 ./mcheck_test
   malloc: using debugging hooks
    *** glibc detected *** ./mcheck_test: free(): invalid pointer: 0x0000000000601010 ***
    *** glibc detected *** ./mcheck_test: malloc: top chunk is corrupt: 0x0000000000601020 ***

Mcheck is not a complete solution and does not detect any errors that may occur 
in memory handling. It detects, however, a significant number of errors and is 
an important feature of glibc.

Memory leaks
------------

A memory leak occurs in two situations:
   
   * a program fails to release a memory area
   * a program loses the reference to a allocated memory area and as a
     consequence can not release it

Memory leaks have the effect of reducing the amount of memory in the system.
Extreme situations can result in consuming the entire memory of the system and 
the inability to run its various applications.

As with the problem of invalid access to memory, the Valgrind utility is very 
useful in detecting program memory leaks.

Valgrind
--------

Valgrind is a suite of utilities used for debugging and profiling. The most 
popular is ``memcheck``, a utility that detects memory errors (invalid access,
memory leaks, etc.). Other utilities in the Valgrind suite are cachegrind,
Callgrind useful for profiling or Helgrind, useful for debugging multithreaded 
programs.

Next, we will only refer to the Memcheck memory error detection tool. 
Specifically, this utility detects the following types of errors:

  * using uninitialized memory
  * read / write from memory after the region has been released
  * reading / writing beyond the end of the allocated area
  * read / write on stack in inappropriate areas
  * memory leaks
  * inappropriate use of malloc / new and free / delete calls 

Valgrind does not require the code of a program to be adjusted, but uses the 
executable (binary) associated with a program directly. On a regular run,
Valgrind will get the argument - --tool to specify the utility used and the 
program that will be checked for memory errors.

In the example below, the program presented in the "mcheck" section is used :

.. code-block:: bash

    so@spook$ valgrind --tool=memcheck ./mcheck_test
    ==17870== Memcheck, a memory error detector.
    ==17870== Copyright (C) 2002-2007, and GNU GPL'd, by Julian Seward et al.
    ==17870== Using LibVEX rev 1804, a library for dynamic binary translation.
    ==17870== Copyright (C) 2004-2007, and GNU GPL'd, by OpenWorks LLP.
    ==17870== Using valgrind-3.3.0-Debian, a dynamic binary instrumentation framework.
    ==17870== Copyright (C) 2000-2007, and GNU GPL'd, by Julian Seward et al.
    ==17870== For more details, rerun with: -v
    ==17870== 
    ==17870== Invalid write of size 4
    ==17870==    at 0x4005B1: main (mcheck_test.c:17)
    ==17870==  Address 0x5184048 is 4 bytes after a block of size 20 alloc'd
    ==17870==    at 0x4C21FAB: malloc (vg_replace_malloc.c:207)
    ==17870==    by 0x400589: main (mcheck_test.c:10)
    ==17870== 
    ==17870== Invalid write of size 4
    ==17870==    at 0x4005C8: main (mcheck_test.c:22)
    ==17870==  Address 0x5184048 is 4 bytes after a block of size 20 free'd
    ==17870==    at 0x4C21B2E: free (vg_replace_malloc.c:323)
    ==17870==    by 0x4005BF: main (mcheck_test.c:19)
    ==17870== 
    ==17870== ERROR SUMMARY: 2 errors from 2 contexts (suppressed: 8 from 1)
    ==17870== malloc/free: in use at exit: 40 bytes in 1 blocks.
    ==17870== malloc/free: 2 allocs, 1 frees, 60 bytes allocated.
    ==17870== For counts of detected errors, rerun with: -v
    ==17870== searching for pointers to 1 not-freed blocks.
    ==17870== checked 76,408 bytes.
    ==17870== 
    ==17870== LEAK SUMMARY:
    ==17870==    definitely lost: 40 bytes in 1 blocks.
    ==17870==      possibly lost: 0 bytes in 0 blocks.
    ==17870==    still reachable: 0 bytes in 0 blocks.
    ==17870==         suppressed: 0 bytes in 0 blocks.
    ==17870== Rerun with --leak-check=full to see details of leaked memory.


Used utility ``memcheck`` for obtaining information memory access.

The recommended option ``-g`` when compiling the executable program to include
debugging information. The running of the above identified Valgrind two errors: 
a code appears in line 17 and line 10 is related to (malloc), while the other
appears in line 22 and is coupled to the line 19 (free)

.. code-block:: c

      v1 = (int *) malloc (5 * sizeof(*v1));
       if (NULL == v1) {
              perror ("malloc");
              exit (EXIT_FAILURE);
       }
 
       /* overflow */
       v1[6] = 100;
 
       free(v1);
 
       /* write after free */
       v1[6] = 100;

The following example is a program with a variety of memory allocation errors:

.. code-block:: bash

    #include <stdlib.h>
    #include <string.h>
     
    int main(void)
    {
    	char buf[10];
    	char *p;
     
    	/* no init */
    	strcat(buf, "al");
     
    	/* overflow */
    	buf[11] = 'a';
     
    	p = malloc(70);
    	p[10] = 5;
    	free(p);
     
    	/* write after free */
    	p[1] = 'a';
    	p = malloc(10);
     
    	/* memory leak */
    	p = malloc(10);
     
    	/* underrun */
    	p--;
    	*p = 'a';
     
    	return 0;
    }

The following are executable behavior obtained from a normal running and a run 
under Valgrind:

.. code-block:: bash
   
    so@spook$ make
    cc -Wall -g    valgrind_test.c   -o valgrind_test
    so@spook$ ./valgrind_test 
    so@spook$ valgrind --tool=memcheck ./valgrind_test
    ==18663== Memcheck, a memory error detector.
    ==18663== Copyright (C) 2002-2007, and GNU GPL'd, by Julian Seward et al.
    ==18663== Using LibVEX rev 1804, a library for dynamic binary translation.
    ==18663== Copyright (C) 2004-2007, and GNU GPL'd, by OpenWorks LLP.
    ==18663== Using valgrind-3.3.0-Debian, a dynamic binary instrumentation framework.
    ==18663== Copyright (C) 2000-2007, and GNU GPL'd, by Julian Seward et al.
    ==18663== For more details, rerun with: -v
    ==18663== 
    ==18663== Conditional jump or move depends on uninitialised value(s)
    ==18663==    at 0x40050D: main (valgrind_test.c:10)
    ==18663== 
    ==18663== Invalid write of size 1
    ==18663==    at 0x400554: main (valgrind_test.c:20)
    ==18663==  Address 0x5184031 is 1 bytes inside a block of size 70 free'd
    ==18663==    at 0x4C21B2E: free (vg_replace_malloc.c:323)
    ==18663==    by 0x40054B: main (valgrind_test.c:17)
    ==18663== 
    ==18663== Invalid write of size 1
    ==18663==    at 0x40057C: main (valgrind_test.c:28)
    ==18663==  Address 0x51840e7 is 1 bytes before a block of size 10 alloc'd
    ==18663==    at 0x4C21FAB: malloc (vg_replace_malloc.c:207)
    ==18663==    by 0x40056E: main (valgrind_test.c:24)
    ==18663== 
    ==18663== ERROR SUMMARY: 6 errors from 3 contexts (suppressed: 8 from 1)
    ==18663== malloc/free: in use at exit: 20 bytes in 2 blocks.
    ==18663== malloc/free: 3 allocs, 1 frees, 90 bytes allocated.
    ==18663== For counts of detected errors, rerun with: -v
    ==18663== searching for pointers to 2 not-freed blocks.
    ==18663== checked 76,408 bytes.
    ==18663== 
    ==18663== LEAK SUMMARY:
    ==18663==    definitely lost: 20 bytes in 2 blocks.
    ==18663==      possibly lost: 0 bytes in 0 blocks.
    ==18663==    still reachable: 0 bytes in 0 blocks.
    ==18663==         suppressed: 0 bytes in 0 blocks.
    ==18663== Rerun with --leak-check=full to see details of leaked memory.


It can be seen that a regular running program does not generate any error.
However, running with Valgrind, errors in three contexts:

   * call strcat(line 10) string is not initialized
   * write memory after free(line 20: p[1] = 'a')
   * underrun (line 28) 

In addition, there is memory leak because of the new call malloc that
associates a new value of p(line 24).

Valgrind is a basic debugging tool. It is easy to use (not intrusive, requiring 
no modification of sources) and allows detection of a large number of 
programming errors that result from poor memory management.

Full information on how to use Valgrind and associated utilities found in the 
pages of documentation Valgrind.

profiling
=========

A profiler is a performance analysis utility that helps the programmer 
determine the bottleneck of a program. This is done by investigating program 
behavior, evaluating memory consumption and the relationship between its 
modules.

perfcounters
------------

Most modern processors offer performance counters that track different types of
hardware events: executed instructions, cache-misses, missed missed
instructions, without affecting the performance of the kernel or applications.
These registers can trigger interruptions when a certain number of events
accumulate and so can be used to analyze the code running on the processor in
question.

The perfcounters subsystem
   * is in the Linux kernel since version 2.6.31 (CONFIG_PERF_COUNTERS=y)
   * replaces oprofile
   * offers support for:
      * hardware events (instructions, cache accesses, bus cycles).
      * software events (page fault, cpu-clock, cpu migrations).
      * tracepoints (eg: sys_enter_open, sys_exit_open).

perf
----

The ``perf`` utility is the user interface perfcounters subsystem. It provides a
git like command line and does not require the existence of a daemon.

Usage:

.. code-block:: bash

  perf [--version] [--help] COMMAND [ARGS]

The most used commands are:

   * ``annotate`` - reads perf.data and display code with perf.data
   * ``list`` - Lists the symbolic names of all types of events that can be watched
     by perf
   * ``lock`` - Analyzes lock events
   * ``record`` - Runs an order and saves the profiling information in the perf.data 
     file
   * ``report`` - Reads perf.data (created by perf record ) and display the profile
   * ``sched`` - Schedule Measurement Tool (latencies)
   * ``stat`` - Run an order and display the statistics posted by the performance 
     counters subsystem
   * ``top`` - Generates and displays real-time information about uploading a system 

perf list
~~~~~~~~~

Displays the symbolic names of all types of events that can be tracked by perf .

.. code-block:: bash

   $ perf list 
   List of pre-defined events (to be used in -e):
    
     cpu-cycles OR cycles                       [Hardware event]
     instructions                               [Hardware event]
    
     cpu-clock                                  [Software event]
     page-faults OR faults                      [Software event]
    
     L1-dcache-loads                            [Hardware cache event]
     L1-dcache-load-misses                      [Hardware cache event]
    
     rNNN                                       [Raw hardware event descriptor]
    
     mem:<addr>[:access]                        [Hardware breakpoint]
    
     syscalls:sys_enter_accept                  [Tracepoint event]
     syscalls:sys_exit_accept                   [Tracepoint event]
   

perf state
~~~~~~~~~~

Run an order and display the statistics posted by the performance counters 
subsystem.

   $ perf stat ls -R /usr/src/linux
    Performance counter stats for 'ls -R /usr/src/linux':
    
            934.512846  task-clock-msecs         #      0.114 CPUs 
                  1695  context-switches         #      0.002 M/sec
                   163  CPU-migrations           #      0.000 M/sec
                   306  page-faults              #      0.000 M/sec
             725144010  cycles                   #    775.959 M/sec 
             419392509  instructions             #      0.578 IPC   
              80242637  branches                 #     85.866 M/sec 
               5680112  branch-misses            #      7.079 %     
             174667968  cache-references         #    186.908 M/sec 
               4178882  cache-misses             #      4.472 M/sec 
    
           8.199187316  seconds time elapsed

perf stat offers the possibility of collecting data by running a program several 
times specifying the -r option.

.. code-block:: bash

   $ perf stat -r 6 sleep 1
    Performance counter stats for 'sleep 1' (6 runs):
    
              1.757147  task-clock-msecs #      0.002 CPUs    ( +-   3.000% )
                     1  context-switches #      0.001 M/sec   ( +-  14.286% )
                     0  CPU-migrations   #      0.000 M/sec   ( +- 100.000% )
                   144  page-faults      #      0.082 M/sec   ( +-   0.147% )
               1373254  cycles           #    781.525 M/sec   ( +-   2.856% )
                588831  instructions     #      0.429 IPC     ( +-   0.667% )
                106846  branches         #     60.806 M/sec   ( +-   0.324% )
                 11312  branch-misses    #     10.587 %       ( +-   0.851% )
           1.002619407  seconds time elapsed   ( +-   0.012% )

Note the most important events listed above.

perf top
~~~~~~~~

Generates and displays real-time information about uploading a system.

.. code-block:: bash

   $ ls -R /home
   $ perf top -p $(pidof ls)
   --------------------------------------------------------------
      PerfTop:     181 irqs/sec  kernel:72.4% (target_pid: 10421)
   --------------------------------------------------------------
                samples  pcnt function             DSO
                _______ _____ ____________________ ___________________
    
                 270.00 15.8% __d_lookup           [kernel.kallsyms]  
                 145.00  8.5% __GI___strcoll_l     /lib/libc-2.12.1.so
                  99.00  5.8% link_path_walk       [kernel.kallsyms]  
                  97.00  5.7% find_inode_fast      [kernel.kallsyms]  
                  91.00  5.3% __GI_strncmp         /lib/libc-2.12.1.so
                  55.00  3.2% move_freepages_block [kernel.kallsyms]  
                  44.00  2.6% ext3_dx_find_entry   [kernel.kallsyms]  
                  41.00  2.4% ext3_find_entry      [kernel.kallsyms]  
                  40.00  2.3% dput                 [kernel.kallsyms]  
                  39.00  2.3% ext3_check_dir_entry [kernel.kallsyms]  

We note that file-handling functions (iterate, find) are the ones that most 
often appear in the perf-top output of the recursive home directory command.

perf record
~~~~~~~~~~~

Run a command and save the profiling information in the perf.data file.

.. code-block:: bash

  $ perf record wget http://elf.cs.pub.ro/so/wiki/laboratoare/laborator-07
   
  [ perf record: Woken up 1 times to write data ]
  [ perf record: Captured and wrote 0.008 MB perf.data (~334 samples) ]
   
  $ ls
  laborator-07  perf.data
  
perf report
~~~~~~~~~~~

Interprets saved data in perf.data after analysis using perf record . Thus for 
the example wget above we have:

.. code-block:: bash

   $ perf report 
   # Events: 13  cycles
   #
   # Overhead  Command      Shared Object  Symbol
   # ........  .......  .................  ......
   #
       86.43%     wget             e8ee21  [.] 0x00000000e8ee21
       11.03%     wget  [kernel.kallsyms]  [k] prep_new_page
        2.37%     wget  [kernel.kallsyms]  [k] sock_aio_read
        0.11%     wget  [kernel.kallsyms]  [k] perf_event_comm
        0.05%     wget  [kernel.kallsyms]  [k] native_write_msr_safe

Exercises
=========

Exercise 1 - Compilation
------------------------

Go to 1-ops/ directory and examine content of ops.c, mul.c and add.c
files. File ops.c, using the functions defined in mul.c and add.c 
performs simple addition and multiplication operations.

Create file Makefile, so you get the source object files mul.o, add.o and
ops.o and then link them to get ops executable. Check the result of addition
and multiplication. Is it correct? Fix the issue.

Stay in the directory -ops/ and use the options ``-D`` define the symbol
HAVE_MATH when compiling the file ops.c. Obtain and run the executable ops. To
use the pow function you must include file math.h and link libm library to the
final executable using option -l of the gcc.

Exercise 2 - Printing order
---------------------------

Go to the ``2-print/`` directory and examine the contents of the ``print.c`` file.
Use the ``make print`` command to compile the print program.

   * Is there any ``Makefile``?
   * What is the order in which console prints are made? Explain the output.
   * Put a ``sleep(5)`` statement before ``return 0``, in the main function and use
     the ``strace -e write ./print`` command to find the explanation.

Exercise 3 - Segmentation fault
------------------------------------------------------

Got to 3-gdb/ directory and examine the source. The program should read a message
from stdin and display it.

  * Compile and run the source.
  * Run the program again using gdb (revisit running a program from gdb section).

To identify exactly where the crash occured use ``backtrace`` command. For details on
gdb commands use the command help:

.. code-block:: bash

   (gdb) help

Change the current frame by frame of function main:

.. code-block:: bash

   (gdb) frame main

Inspect the variable buf:

.. code-block:: bash

   (gdb) print buf

Now we want to see why is ``buf = NULL``, following these steps:

   * Kill the current process: ``(gdb)  kill``
   * Set a breakpoint at the beginning of the function main:
     ``(gdb) break main``
   * run the program and inspect the value ``buf`` before and after the malloc
     function call (use ``next`` to move to the next statement).
   * explain the source of the error, then fix it.

Exercise 4 - Valgrind, memory access
-----------------------------------

Go to the 4-flowers/ folder and analyze the contents of the flowers.c.
Compile the flowers.c file and run the executable flowers. What happens? Use
valgrind with the --tool=memcheck option. Show the value of the third element
of the flowers array, flowers[2] .

Exercise 5 - Valgrind, memory allocation
----------------------------------------

Go to 5-struct folder and analyze the contents of the struct.c file.

The function allocate_flowers allocates memory for ``no`` elements of type
flower_info, and function free_flowers release allocated memory from function
allocate_flowers.
   * Run the program. Did you notice any errors?
   * Correct any errors. You can use --tool=memcheck option for valgrind.

Exercise 6 - Row / Column major order
-------------------------------------
Using the perf we want to determine whether the C language is
column-major or row-major.

Go to 6-major directory and fill the ``row.c`` so that it increments the
elements of a matrix on lines, then fill out the columns.c so as to increment
the elements of the matrix on columns.

Determine the number of missed caches compared to the number of cached accesses
using the perf stat to track the ``L1-dcache-load-misses``. To see the available
events, use the ``perf list`` command. Use the -e option of the perf utility to
specify a specific event to watch (see the perfcounters section).

Exercise 7 - Busy
-----------------

Go to 7-busy directory and inspect the ``busy.c`` file. Run the ``busy`` program 
and analyze system load using ``perf top`` command. What function does the
system load seems to be?


Exercise 8 - Searching for a string
-----------------------------------

Go to the 8-find-char/ directory and analyze the contents of the find-char.c.
Compile the find-char.c file and run the executable.

Identify using perf record and perf report what is the most time-consuming 
processor function and try to improve the performance of the program.


Exercise 9 - Working with the stack
-----------------------------------

Go to 9-bad-stack/ directory and examine bad_stack.c file. Compile and run the program.
Notice that in the main function the first print of str is correct but the second time
it isnt't. Can you explain why? Modify the source code such that the ``lab_so`` variable
could be accessible after function return.

Exercise 10 - Endianess
-----------------------

Go to 10-endian/ directory and inspect endian.c source file. Make use of ``w`` variable
to print each byte of the number ``0xDEADBEEF``.

What type of architecture are you running on? (big-endian or little-endian, see here for details).

Exercise 11 - mcheck
--------------------

Go to 11-trim/ directory and inspect the trim.c program, compile and run the executable
``trim``.

Try to detect the problem using gdb. Then, use mcheck to detect the problem and correct 
it (see section mcheck laboratory).  Run mcheck as follows:

.. code-block:: bash

   MALLOC_CHECK_=1 ./trim

..
   Exercise 7 - Buffer overflow exploit
   ------------------------------------
   
   Go to the 7-exploit/ directory and analyze the contents of the exploit.c file. 
   Use the make command to compile the exploit executable. Identify a problem in 
   the read_name function.
   
   Use gdb to investigate the stack before making the read call.
   
   .. code-block:: bash
   
      student@spook:~ gdb ./exploit
      (gdb) break read_name
      (gdb) run
      # print the addresses of access and name vars
      (gdb) print/x &access
      (gdb) print/x &name
   
   Notice that the difference between the address of the access variable and the
   name buffer is 0x10 (16) bytes, which means that the access variable is 
   immediately at the end of the data in the name buffer.
   
   Using your information, build a convenient input that you can give to the 
   exploit executable so that it displays the "Good job, you hacked me!" string.
   
   To generate non-printable characters, you can use the Python interpreter: 
   python -c .
   
   .. code-block:: bash
   
     student@spook:~ python -c 'print "A"*8 + "\x01\x00\x00\x00"' | ./exploit
   
   The above command will generate 8 bytes with the value 'A' (ASCII code 0x41),
   a byte with the value 0x01 and another 3 bytes with the value 0x00 and will
   provide it to the executable exploit stdin . Note that the data is structured 
   in small endian memory, so if the last 4 bytes will overwrite an address, it
   will be interpreted as 0x00000001, no 0x01000000.
   
   Exercise 8 - Trace the mystery
   ------------------------------
   
   Go to the 8-mystery/ undefined directory where you find the mystery executable.
   Investigate and explain what it is doing. Review the strace section.
   
   
