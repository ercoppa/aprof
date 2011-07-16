LIB = `pkg-config --libs glib-2.0` `pkg-config --cflags glib-2.0`
# Opzioni usate da valgrind nella compilazione 
W_VALGRIND = -Wmissing-prototypes -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-declarations 

driver:	driver.c union_find.c ../pool/pool.c 
	gcc -Wall driver.c union_find.c ../pool/pool.c  -o driver -DGLIBC

test:	union_test.c union_find.c ../pool/pool.c
	gcc -Wall union_test.c union_find.c ../pool/pool.c -o test -DGLIBC -DDEBUG

clean:
	rm driver; rm test; rm *.o

