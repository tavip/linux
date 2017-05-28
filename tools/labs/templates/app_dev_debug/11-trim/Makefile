CC = gcc
CPPFLAGS = -I../utils
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: trim

trim: trim.o

trim.o: trim.c

clean:
	-rm -f *.o *~ trim
