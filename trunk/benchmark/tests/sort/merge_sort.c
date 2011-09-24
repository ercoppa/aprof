#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#if 0
void stampa(int a, int b) {
//	printf("A: %d - B: %d\n", a, b);
}
#endif

void merge(int v[], int temp[], int a, int m, int b) {
    int k, i = a, j = m;
    for (k = a; k < b; k++)
        if (i < m && j < b)
            if (v[i] < v[j]) temp[k] = v[i++];
            else temp[k] = v[j++];
        else if (i < m) temp[k] = v[i++];
        else temp[k] = v[j++];
//      int q = 0;
//      for (q = a; q < b; q++) temp[q] = v[q]; 
//        memcpy(v+a, temp+a, (b-a)*sizeof(int));
}

void merge_sort_ric(int v[], int temp[], int a, int b) {

//   printf("A: %d - B: %d\n", a, b);
//    stampa(a, b);

    int m;
    if (a + 1 >= b) return;
    m = (a + b)/2;
    merge_sort_ric(v, temp, a, m);
    merge_sort_ric(v, temp, m, b);
//    merge(v, temp, a, m, b);
//    int j = 0;
//    for (j = a; j < b ; j++) v[j]++; 
}

void merge_sort(int v[], int n) {
    int* temp = (int*)malloc(n*sizeof(int));
    if (temp == NULL) exit(1);
//    memcpy(v, temp, n*sizeof(int));
    merge_sort_ric(v, temp, 0, n);
    free(temp);
}

#define N 1024*10

int main() {

//    int a = 0, b = 0;
//    memcpy(&a, &b, 1);

    int* v = malloc(sizeof(int)*N);
    srand(time(0));
    int j = 0;
    for (j = 0; j < N; j++)
	v[j] = rand() % N;


    merge_sort(v, N);

    free(v);

    return 0;
}


