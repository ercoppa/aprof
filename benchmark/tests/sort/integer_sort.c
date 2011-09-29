#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void integer_sort(int v[], int n, int k) {
    unsigned* c = calloc(k, sizeof(unsigned)), i, j;
    for (i = 0; i < n; i++)
	if (v[i] < k) c[v[i]]++;
	else exit((printf("errore\n"), 1));
    for (i = j = 0; i < k; i++)
	while (c[i]-- > 0) v[j++] = i;
    free(c);
}

int main() {

    int v[1024*100];
    srand(time(0));
    int j = 0, i = 0;
    int n = sizeof(v)/sizeof(int);
    
    for (i = 10; i < n; i+=100) {
    
	for (j = 0; j < i; j++)
		v[j] = rand() % i;

	integer_sort(v, i, i+1);
    }
    return 0;
}

