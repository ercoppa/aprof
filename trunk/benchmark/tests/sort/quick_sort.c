#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print(int v[], int n, char* msg) {
    int i;
    printf("%s (n=%d): ", msg, n);
    for (i=0; i<n; i++) printf("%d ", v[i]);
    printf("\n");
}

int partition(int v[], int a, int b) {
	int pivot = a;
	for (;;) {
		while (a < b && v[a] <= v[pivot]) a++;
		while (v[b] > v[pivot]) b--;
		if (a >= b) break;
		int temp = v[a];
		v[a] = v[b];
		v[b] = temp;
	}
	if (v[b] != v[pivot]) {
		int temp = v[b];	
		v[b] = v[pivot];
		v[pivot] = temp;
	}
	return b;
}

void quick_sort_ric(int v[], int a, int b) {
    int m;
    if (a >= b) return;
    m = partition(v, a, b);
    quick_sort_ric(v, a, m-1);
    quick_sort_ric(v, m+1, b);
}

void quick_sort(int v[], int n) {
    quick_sort_ric(v, 0, n-1);
}

void quick_sort_ric2(int v[], int a, int b) {
    int m;
    if (a >= b) return;
    m = partition(v, a, b);
    quick_sort_ric2(v, a, m-1);
    quick_sort_ric2(v, m+1, b);
}

void quick_sort2(int v[], int n) {
    quick_sort_ric2(v, 0, n-1);
}


int main() {
	
	int v[1000];
	srand(time(0));
	int j = 0;
	int n = sizeof(v)/sizeof(int);
	for (j = 0; j < sizeof(v)/sizeof(int); j++)
		v[j] = rand() % n;

	//print(v, n, "unsorted array");

	quick_sort(v, n);
	quick_sort2(v, n);

	//print(v, n, "sorted array");
	return 0;
}

