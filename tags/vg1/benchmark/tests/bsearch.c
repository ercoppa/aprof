#include <stdio.h>
#include <stdlib.h>

int compareints (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

int main (int argc, char ** argv)
{
	int n = atoi(argv[1]);
	int i;
	for (i=0; i<n; i++) {
		int size = rand() % 100000;
		int array[size];
		int j;
		for (j=0; j<size; j++) array[j] = j;
  		int key = rand() % size;
  		bsearch (&key, array, size, sizeof (int), compareints);
  	}
  	return 0;
}
