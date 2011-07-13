LIB = `pkg-config --libs glib-2.0` `pkg-config --cflags glib-2.0`
# Opzioni usate da valgrind nella compilazione 
W_VALGRIND = -Wmissing-prototypes -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-declarations 

driver:	driver.c union_find.c pool.c uf_alloc.c hashtable.c
	gcc -Wall driver.c union_find.c pool.c uf_alloc.c hashtable.c -o driver -DGLIBC

driver_glib:	driver.c union_find.c pool.c uf_alloc.c
	gcc -Wall driver.c union_find.c pool.c uf_alloc.c $(LIB) -o driver_glib -DGLIBC -DGLIB

hash:	hashtable.c hash_test.c
	gcc -Wall hashtable.c hash_test.c -o hash -DGLIBC

test:	union_test.c union_find.c pool.c uf_alloc.c hashtable.c
	gcc -Wall union_test.c union_find.c pool.c uf_alloc.c hashtable.c -o test -DGLIBC -DDEBUG

clean:
	rm driver; rm test; rm driver_glib; rm hash; rm *.o

