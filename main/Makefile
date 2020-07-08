CC=gcc
CFLAGS=-Wall -ansi -std=c99 -pedantic -m32 -fno-pic -fno-stack-protector -nostartfiles -nostdlib -nodefaultlibs

all: apic.o cpu.o dma.o intr.o keyboard.o kmalloc.o lterm.o memory.o semaphore.o system.o time.o

apic.o: apic.h apic.c cpu.h cpu.o memory.h ../globals.h ../asm/i386.h ../asm/types.h
	$(CC) $(CFLAGS) -c apic.c -o apic.o

cpu.o: cpu.h cpu.c ../globals.h ../asm/i386.h ../asm/types.h ../std/string.h
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

dma.o: dma.h dma.c ../globals.h ../asm/i386.h ../asm/types.h system.h
	$(CC) $(CFLAGS) -c dma.c -o dma.o

intr.o: intr.h intr.c ../globals.h ../asm/i386.h ../asm/types.h ../asm/segments.h lterm.h system.h
	$(CC) $(CFLAGS) -c intr.c -o intr.o

keyboard.o: keyboard.h keyboard.c ../globals.h ../asm/i386.h ../asm/types.h ../structures/buf.h system.h
	$(CC) $(CFLAGS) -c keyboard.c -o keyboard.o

kmalloc.o: kmalloc.h kmalloc.c ../globals.h ../asm/types.h ../asm/segments.h ../std/math.h err.h memory.h
	$(CC) $(CFLAGS) -c kmalloc.c -o kmalloc.o

lterm.o: lterm.h lterm.c ../globals.h ../asm/types.h ../std/string.h ../std/math.h system.h
	$(CC) $(CFLAGS) -c lterm.c -o lterm.o

memory.o: memory.h memory.c ../globals.h ../asm/types.h intr.h lterm.h system.h
	$(CC) $(CFLAGS) -c memory.c -o memory.o

semaphore.o: semaphore.h semaphore.c ../globals.h ../asm/types.h
	$(CC) $(CFLAGS) -c semaphore.c -o semaphore.o

system.o: system.h system.c ../globals.h ../asm/i386.h ../asm/types.h
	$(CC) $(CFLAGS) -c system.c -o system.o

time.o: time.h time.c ../globals.h ../asm/i386.h intr.h
	$(CC) $(CFLAGS) -c time.c -o time.o

clean:
	rm -f *.o

.PHONY: clean all
