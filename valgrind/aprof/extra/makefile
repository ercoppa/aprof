CFLAGS = -Wall -Werror
CC     = gcc

all: ../data-common.h valgrind-types.h hashtable.o aprof-helper.c 
	$(CC) -o aprof-helper $(CFLAGS) aprof-helper.c hashtable.o

hashtable.o: ../hashtable/hashtable.h ../hashtable/hashtable.c
	$(CC) -c $(CFLAGS) ../hashtable/hashtable.c -DEXTERNAL

clean:
	rm aprof-helper hashtable.o
