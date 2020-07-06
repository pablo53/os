CC=gcc
CFLAGS=-Wall -ansi -std=c99 -pedantic -m32 -fno-pic -fno-stack-protector

all: ide.o

ide.o: ide.h ide.c ../globals.h ../asm/i386.h ../main/time.h ../main/err.h
	$(CC) $(CFLAGS) -c ide.c -o ide.o

clean:
	rm -f *.o

.PHONY: clean all
