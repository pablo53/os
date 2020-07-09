CC=gcc
AS=as
LD=ld
CFLAGS=-Wall -ansi -std=c99 -pedantic -O0 -m32 -fno-pic -fno-stack-protector -nostartfiles -nostdlib -nodefaultlibs
LDFLAGS=-m elf_i386

all: all.o

all.o: string.o math.o
	$(LD) $(LDFLAGS) -r $^ -o $@
	rm -f $^

string.o: string.h string.c
	$(CC) $(CFLAGS) -c string.c -o string.o

math.o: math.h math_defs.h math.c
	$(CC) $(CFLAGS) -c math.c -o math.o

clean:
	rm -f *.o

.PHONY: clean all
