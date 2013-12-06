CFLAGS = -g -Wall -Werror -std=gnu99
CC     = gcc

all:  aprof-helper

aprof-helper: ../data-common.h aprof-helper.h hashtable.o merge.o report.o util.o aprof-helper.c
	$(CC) -o aprof-helper $(CFLAGS) aprof-helper.c hashtable.o merge.o report.o util.o -DEXTERNAL

install: aprof-helper
	cp aprof-helper ../../inst/bin/

hashtable.o: ../hashtable/hashtable.h ../hashtable/hashtable.c aprof-helper.h 
	$(CC) -c $(CFLAGS) ../hashtable/hashtable.c -DEXTERNAL

merge.o: ../merge.c aprof-helper.h ../data-common.h
	$(CC) -c $(CFLAGS) ../merge.c -DEXTERNAL

report.o: ../report.c aprof-helper.h ../data-common.h
	$(CC) -c $(CFLAGS) ../report.c -DEXTERNAL

util.o: ../util.c aprof-helper.h 
	$(CC) -c $(CFLAGS) ../util.c -DEXTERNAL

clean:
	rm aprof-helper hashtable.o merge.o report.o
