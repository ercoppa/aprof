#include <stdio.h>
#include <time.h>
#include <stdlib.h> 

void print(int v[], int n, char* msg) {
	int i;
	printf("%s (n=%d): ", msg, n);
    for (i=1; i<=n; i++) printf("%d ", v[i]);
	printf("\n");
}

void fix_heap(int v[], int i, int n) {
	int m = 2*i;
	if (m > n) return;
	if (m+1 <= n && v[m] < v[m+1]) m++;
	if (v[i] >= v[m]) return;
	int temp = v[i];
	v[i] = v[m];
	v[m] = temp;
	fix_heap(v, m, n);
}

void heapify(int v[], int i, int n) {
	if (2*i > n) return;
	heapify(v, 2*i, n);
	heapify(v, 2*i+1, n);
	fix_heap(v, i, n);
}

void heap_sort(int v[], int n) {
	heapify(v, 1, n);
	//print(v, n, "after heapify");
	while (n > 1) {
		int max = v[1];
		v[1] = v[n];
		v[n] = max;
		n--;
		fix_heap(v, 1, n);
	}
}

int main() {

	int v[1024*100];
	srand(time(0));
	int j = 0;
	int n = sizeof(v)/sizeof(int);
	for (j = 0; j < n; j++)
		v[j] = rand() % n;

    heap_sort(v, n);
	
    return 0;
}

