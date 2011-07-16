#include "suf.h"

StackUF * SUF_create(void)
{
	StackUF * s = VG_(calloc)("suf pm", sizeof(struct SPM), 1);
	if (s == NULL) return NULL;
	
	return s;
}

void SUF_destroy(StackUF * suf)
{

	UWord i = 0;
	while (i < SSM_SIZE) {
		if (suf->table[i] != NULL) 
			VG_(free)(suf->table[i]);
		i++;
	}
	VG_(free)(suf);
}

UWord SUF_insert(StackUF * suf, UWord addr, UWord rid)
{
	UWord i = addr >> 16;
	UWord j = (addr & 0xffff) / 4;
	if (suf->table[i] == NULL) {
		
		suf->table[i] = VG_(calloc)("suf sm", sizeof(SSM), 1);
		if (suf->table[i] == NULL) failure("SUF sm not allocable");
		
		suf->table[i]->table[j] = rid;
		return 0;
		
	}
	
	UWord old = suf->table[i]->table[j];
	if (old < rid)
		suf->table[i]->table[j] = rid;
	return old;

}

UWord SUF_lookup(StackUF * suf, UWord addr)
{
	UWord i = addr >> 16;
	if (suf->table[i] == NULL) return 0;
	
	UWord j = (addr & 0xffff) / 4;
	return suf->table[i]->table[j];
}
