#include <stdio.h>
#include <stdlib.h>

unsigned long fib2(int n) {
	int i;
	unsigned long *f, risultato;
	
	f = (unsigned long*)malloc(n*sizeof(unsigned long));
	if (f==NULL) exit((printf("errore di allocazione memoria\n"), 1));

	f[0]=f[1]=1;	
	for (i=2; i<n; i++) f[i] = f[i-1] + f[i-2];	
	risultato = f[n-1];

	free(f);
	return risultato;
}

int main(int argc, char* argv[]) {
    int i, n = atoi(argv[1]);
    unsigned long f;
    
    for (i=0; i<1000000; i++) f = fib2(n);
    printf("F(%d)=%lu\n", n, f);
}

