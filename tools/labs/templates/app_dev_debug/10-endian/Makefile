CC = gcc
CPPFLAGS = -I../utils
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: endian

endian: endian.o

endian.o: endian.c

clean:
	-rm -f *.o *~ endian
