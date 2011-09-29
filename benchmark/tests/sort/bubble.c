#include <stdlib.h> 
#include <stdio.h>
#include <time.h>
 
#define uint32 unsigned int
 
typedef int (*CMPFUN)(int, int);
 
void ArraySort(int This[], CMPFUN fun_ptr, uint32 ub)
{
  /* bubble sort */

  uint32 indx;
  uint32 indx2;
  int temp;
  int temp2;
  int flipped;

  if (ub <= 1)
    return;

  indx = 1; 
  do
  {
    flipped = 0;
    for (indx2 = ub - 1; indx2 >= indx; --indx2)
    {
      temp = This[indx2];
      temp2 = This[indx2 - 1];
      if ((*fun_ptr)(temp2, temp) > 0)
      {
        This[indx2 - 1] = temp;
        This[indx2] = temp2;
        flipped = 1;
      }
    }
  } while ((++indx < ub) && flipped);
}
 

 
void fill_array(int array[], int size)
{
  int indx;

  for (indx=0; indx < size; ++indx)
  {
    array[indx] = rand();
  }
}
 
int cmpfun(int a, int b)
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else
    return 0;
}
 
int main(int argc, char ** argv)
{
  int i;
  int array[1024];
  int n = sizeof(array)/sizeof(int);
  srand(time(0));

  for (i = 0; i < n; i+=10) { 
    fill_array(array, i);
    ArraySort(array, cmpfun, i);
  }

  return(0);
}
 
 
