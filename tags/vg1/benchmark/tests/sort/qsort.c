#include <stdlib.h> 
#include <stdio.h>
 
 
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

void fill_array(int array[], int size)
{
  int indx;

  for (indx=0; indx < size; ++indx)
  {
    array[indx] = rand();
  }
}

int main(int argc, char ** argv)
{
  int i;
  int n = atoi(argv[1]);

  for (i = 0; i < n; i++)
  { 
  	int size = rand() % 1000;
  	int array[size];
    fill_array(array, size);
    qsort (array, size, sizeof(int), compare);
  }

  return(0);
}
