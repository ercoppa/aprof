#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void merge(int v[], int temp[], int a, int m, int b) {
    int k, i = a, j = m;
    for (k = a; k < b; k++)
        if (i < m && j < b)
            if (v[i] < v[j]) temp[k] = v[i++];
            else temp[k] = v[j++];
        else if (i < m) temp[k] = v[i++];
        else temp[k] = v[j++];
    memcpy(v+a, temp+a, (b-a)*sizeof(int));
}

void merge_sort_ric(int v[], int temp[], int a, int b) {

    int m;
    if (a + 1 >= b) return;
    m = (a + b)/2;
    merge_sort_ric(v, temp, a, m);
    merge_sort_ric(v, temp, m, b);
    merge(v, temp, a, m, b);
    
}

void merge_sort(int v[], int n) {
    int* temp = (int*)malloc(n*sizeof(int));
    if (temp == NULL) exit(1);
    merge_sort_ric(v, temp, 0, n);
	free(temp);
}

int main() {

    int v[1024*100];
    srand(time(0));
    int j = 0;
    int n = sizeof(v)/sizeof(int);
    for (j = 0; j < n; j++)
	v[j] = rand() % n;
	
    merge_sort(v, n);

    return 0;
}

