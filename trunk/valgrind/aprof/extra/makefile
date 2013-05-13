CFLAGS = -Wall -Werror -std=gnu99
CC     = gcc

all:  aprof-helper

aprof-helper: ../data-common.h valgrind-types.h hashtable.o aprof-helper.c
	$(CC) -g -o aprof-helper $(CFLAGS) aprof-helper.c hashtable.o

install: aprof-helper
	cp aprof-helper ../../inst/bin/

hashtable.o: ../hashtable/hashtable.h ../hashtable/hashtable.c
	$(CC) -g -c $(CFLAGS) ../hashtable/hashtable.c -DEXTERNAL

clean:
	rm aprof-helper hashtable.o
