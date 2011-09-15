#include <stdio.h>
#include <stdlib.h>

unsigned long fib3(int n) {
	unsigned long i, pen, ult, corr;
	pen = 0;
	ult = corr = 1;
    for (i=3; i<=n; i++) {
        pen = ult;
        ult = corr;
        corr = pen + ult;
    }
	return corr;
}

int main(int argc, char* argv[]) {
    int i, n = atoi(argv[1]);
    unsigned long f;
    
    for (i=0; i<1000000; i++) f = fib3(n);
    printf("F(%d)=%lu\n", n, f);
}

