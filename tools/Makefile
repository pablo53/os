CC=gcc
CFLAGS=-Wall -ansi -std=c99 -pedantic

all: algnfl asecfl cdboot ffsize fabss

algnfl: algnfl.c util.h util.o
	$(CC) $(CFLAGS) algnfl.c util.o -o algnfl

asecfl: asecfl.c
	$(CC) $(CFLAGS) asecfl.c -o asecfl

ffsize: ffsize.c util.h util.o
	$(CC) $(CFLAGS) ffsize.c util.o -o ffsize

fabss: fabss.c util.h util.o
	$(CC) $(CFLAGS) fabss.c util.o -o fabss

cdboot: cdboot.o util.o
	$(CC) $(CFLAGS) cdboot.o util.o -o cdboot

cdboot.o: util.h cdboot.h cdboot.c
	$(CC) $(CFLAGS) -c cdboot.c -o cdboot.o

util.o: util.h util.c
	$(CC) $(CFLAGS) -c util.c -o util.o

clean:
	rm -f algnfl asecfl cdboot ffsize fabss *.o

.PHONY: clean all
