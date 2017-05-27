CC = gcc
CPPFLAGS = -I../utils -w

.PHONY: all clean

all: bad_stack

bad_stack: bad_stack.o

bad_stack.o: bad_stack.c ../utils/utils.h

clean:
	-rm -f *.o *~ bad_stack
