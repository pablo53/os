CC=gcc
AS=as
LD=ld
CFLAGS=-Wall -ansi -std=c99 -pedantic -O0 -m32 -fno-pic -fno-stack-protector -nostartfiles -nostdlib -nodefaultlibs
AFLAGS=--32
LDFLAGS=-m elf_i386

all: boot.img tools/cdboot
	rm -f os.iso
	tools/cdboot boot.img os.iso

vmdk: os.vmdk

os.vmdk: boot.img
	qemu-img convert boot.img -O vmdk os.vmdk

boot.img: os all.bin tools/algnfl tools/asecfl
	cat os > boot.img
	cat all.bin >> boot.img
	tools/algnfl boot.img
	tools/asecfl boot.img

os: os.o
	$(LD) $(LDFLAGS) --oformat binary -Ttext 0 -o os os.o

os.o: os.s os_const.s
	$(AS) $(AFLAGS) -o os.o os.s

os_const.s: all.bin tools/ffsize tools/fabss all.mmap.txt
	$(shell grep '^\.bss\s*0x[0-9a-fA-F]*\s*0x[0-9a-fA-F]*\s*' all.mmap.txt | tr -s ' ' | cut -d' ' -f2-3 | sed 's:^:tools/fabss all.bin :')
	tools/ffsize -s512 all.bin "\nC_SECTORS = %d\n" > os_const.s
	tools/ffsize -a8 all.bin "\nC_C_CODE_LENGTH = %d\n" >> os_const.s
	echo "\nC_ENTRY_POINT = $(shell grep '^\s*0x[0-9a-fA-F]*\s*_start\s*' all.mmap.txt | tr -s ' ' | cut -d' ' -f2)" >> os_const.s

all.bin all.mmap.txt: all.o
	$(LD) $(LDFLAGS) -Map all.mmap.txt --oformat binary -Ttext 0 -o all.bin all.o

all.o: main.o main/all.o std/all.o
	$(LD) $(LDFLAGS) -r $^ -o $@

main.o: main.c main.h
	$(CC) $(CFLAGS) -c main.c -o main.o

std/all.o:
	(cd std; make)

main/all.o:
	(cd main; make)

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
	rm -f boot.img os.iso
	rm -f *.vmdk

test:
	objdump --target=binary --architecture=i8086 --disassemble-all os

.PHONY: clean cleanup test all info vmdk
