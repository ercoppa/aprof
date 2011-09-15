#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void selection_sort(int v[], int n) {
    int k, j;
    for (k = 0; k < n-1; k++) {
        int min = k;
        for (j = k+1; j < n; j++)
            if (v[j] < v[min]) min = j;
        if (min != k) {
            int temp = v[k];
            v[k] = v[min];
            v[min] = temp;
        }
    }
}

int main() {

    int v[1024];
	srand(time(0));
	int j = 0, i = 0;
	int n = sizeof(v)/sizeof(int);
    
    for (i = 0; i < n; i+=10) {
        
        for (j = 0; j < i; j++)
            v[j] = rand() % n;
            
        selection_sort(v, i);
    
    }

    return 0;
}

