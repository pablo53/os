CC=gcc
AS=as
LD=ld
CFLAGS=-Wall -ansi -std=c99 -pedantic -O0


all: boot.img tools/cdboot
	rm -f iso.img
	tools/cdboot boot.img iso.img

vmdk: os.vmdk

os.vmdk: boot.img
	qemu-img convert boot.img -O vmdk os.vmdk

boot.img: os main.bin tools/algnfl tools/asecfl
	cat os > boot.img
	cat main.bin >> boot.img
	tools/algnfl boot.img
	tools/asecfl boot.img

os: os.o
	$(LD) --oformat binary -Ttext 0 -o os os.o

os.o: os.s os_const.s
	$(AS) -o os.o os.s

os_const.s: main.bin tools/ffsize tools/fabss main.mmap.txt
	$(shell grep '^\.bss\s*0x[0-9a-fA-F]*\s*0x[0-9a-fA-F]*\s*' main.mmap.txt | tr -s ' ' | cut -d' ' -f2-3 | sed 's:^:tools/fabss main.bin :')
	tools/ffsize -s512 main.bin "\nC_SECTORS = %d\n" > os_const.s
	tools/ffsize -a8 main.bin "\nC_C_CODE_LENGTH = %d\n" >> os_const.s

main.bin main.mmap.txt: main.o
	$(LD) -Map main.mmap.txt --oformat binary -Ttext 0 -o main.bin main.o

main.o: main.c main.h main/lterm.c main/lterm.h main/intr.h main/intr.c \
main/time.c main/time.h main/system.c main/system.h main/semaphore.c main/semaphore.h \
main/keyboard.c main/keyboard.h main/memory.c main/memory.h main/dma.c main/dma.h \
std/string.c std/string.h std/math.c std/math.h main/kmalloc.c main/kmalloc.h \
main/cpu.c main/cpu.h main/apic.c main/apic.h main/err.h dev/ide.c dev/ide.h \
asm/types.h asm/i386.h asm/segments.h asm/descriptor.h
	$(CC) $(CFLAGS) -c main.c -o main.o

tools/algnfl:
	(cd tools; make)

tools/asecfl:
	(cd tools; make)

tools/cdboot:
	(cd tools; make)

tools/ffsize:
	(cd tools; make)

tools/fabss:
	(cd tools; make)

info: main.bin
	objdump --target=binary --architecture=i386 --disassemble-all main.bin
	objdump -x main.o

cleanup:
	rm -f *.o os os_const.s *.bin *.mmap.txt
	(cd tools; make clean)
	(cd std; make clean)
	(cd main; make clean)

clean: cleanup
	rm -f boot.img iso.img
	rm -f *.vmdk

test:
	objdump --target=binary --architecture=i8086 --disassemble-all os

.PHONY: clean cleanup test all info vmdk
