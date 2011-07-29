#include <stdio.h>
#include <stdlib.h>

void foo(char * p, int j) {
	
	int i = 0;
	while (i++ < 1024)
		p[i] = p[i] + i*2;
	
	if (j > 0)
		foo(p, j-1);

}

int main() {
	
	char * p = malloc(10000);
	
	int i = 0;
	while (i++ < 1024)
		p[i] = p[i] + i;

	foo(p, 10);

	return 0;
}
