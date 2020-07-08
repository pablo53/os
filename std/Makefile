CC=gcc
AS=as
LD=ld
CFLAGS=-Wall -ansi -std=c99 -pedantic -O0 -m32 -fno-pic -fno-stack-protector -nostartfiles -nostdlib -nodefaultlibs

all: string.o math.o

string.o: string.h string.c
	$(CC) $(CFLAGS) -c string.c -o string.o

math.o: math.h math.c
	$(CC) $(CFLAGS) -c math.c -o math.o

clean:
	rm -f *.o

.PHONY: clean all
