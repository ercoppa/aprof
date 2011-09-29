#include <stdlib.h>
#include <stdio.h>

int logar(int * v, int n) {
	int i, sum = 0;
	for (i = n-1; i > 0; i /=2) sum += v[i];
	return sum;
}

int quadratic(int * v, int n) {
	int i, j, sum = 0;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++) sum += v[j];
	return sum;
}

int linear(int * v, int n) {
	int j, sum = 0;
	for (j = 0; j < n; j++) sum += v[j];
	return sum;
}

int main(int argc, char * argv[]) {
	
	if (argc == 1)
	
	double i = 1.0;
	int j = 0;
	int * p = malloc(sizeof(int) * 10000000);
	//1000000*5
	//int * a = NULL;
	//int * old = NULL;
	while (i < 10000000) {
		
		for (j = 0; j < 1; j++) {
			
			if (alg == 1)
				linear(p, (int)i);
			else if (alg == 2)
				quadratic(p, (int)i);
			else if (alg == 3)
				logar(p, (int)i);
		
		}
		i = i*1.20;
		
		/*
		a = realloc(a, sizeof(int) * i);
		if (a != old) printf("Nuovo spazio\n");
		old = a;
		linear(a, i);
		printf("Lettura: %d\n", sizeof(int) * i);
		
		j++;
		
		i = i * 2;
		*/
	}
	
	//printf("Attivazioni: %d\n", j);
	
	return 0;
}
