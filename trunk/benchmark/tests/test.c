#include <stdlib.h>

int main(){
	unsigned char * a = malloc(10000000);
	unsigned long long b, c;
	for (b = 0; b < 10000000; b++)
		a[b]++;
	
	b = 0;
	c = 0;
	while (b++ < 4000000000)
		c++;
	
	for (b = 0; b < 10000000; b++)
		a[b]++;
	
	return 0;
}
