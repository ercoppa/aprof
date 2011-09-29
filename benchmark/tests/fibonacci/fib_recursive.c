#include <stdio.h>
#include <stdlib.h>

unsigned long fib1(int n) {
    if (n < 3) return 1;
    return fib1(n-1) + fib1(n-2);
}

int main(int argc, char* argv[]) {
    int n = atoi(argv[1]);
    unsigned long f = fib1(n);
    printf("F(%d)=%lu\n", n, f);
}

