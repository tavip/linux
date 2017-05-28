CC = gcc
CPPFLAGS = -I../utils -w

.PHONY: all clean

all: bad_stack

bad_stack: bad_stack.o

bad_stack.o: bad_stack.c

clean:
	-rm -f *.o *~ bad_stack
