#include <stdio.h>
#include <stdlib.h>
#include "union_find.h"

#define ADDR_MULTIPLE 4

int main(int argc, char * argv[]) {
	
	if (argc != 2) {
		printf("Usage:\n\t %s tracefile\n", argv[0]);
		exit(1);
	} 
	
	FILE * trace = fopen(argv[1], "r");
	if (trace == NULL) {
		printf("Invalid trace path\n");
		exit(1);
	} 
	
	UnionFind * uf = UF_create();
	
	char type;
	unsigned int addr;
	unsigned int size;
	unsigned int stack_depth = 0;
	//int n = 0;
	do {
		
		int f = fscanf(trace, "%c:%u:%u\n", &type, &size, &addr);
		if (f == EOF) break;
		else if (f != 3) {
			printf("Invalid trace: read %d input\n", f);
			if (f == 1) printf("Read type: %c\n", type);
			exit(1);
		}
		
		if (type == 'a') {
			
			//printf("accessed: %u\n", addr);
			
			#if ADDR_MULTIPLE > 1
			unsigned int diff = addr & (ADDR_MULTIPLE-1);
			addr -= diff;
			if (size + diff < ADDR_MULTIPLE) 
				size = 1;
			else if (((size + diff) % ADDR_MULTIPLE) == 0)
				size = (size + diff) / ADDR_MULTIPLE;
			else
				size = 1 + ((size + diff) / ADDR_MULTIPLE);
			#endif
			
			int i;
			for (i = 0; i < size; i++)
				UF_insert(uf, addr+i, stack_depth);
		
		} else if (type == 'i') {
			
			//printf("enter function: %u\n", addr);
			stack_depth++;
			
		} else if (type == 'o') {
			
			//printf("exit function: %u\n", addr);
			//if (n++ == 0) UF_print(uf);
			UF_merge(uf, stack_depth);
			stack_depth--;
			
		} else {
			printf("Invalid type\n");
			exit(1);
		}
		
	} while(1);
	
	UF_destroy(uf);
	
	return 0;
}
