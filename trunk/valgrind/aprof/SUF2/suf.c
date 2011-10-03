#include "aprof.h"

#define CHECK_ADDR_OVERFLOW(x) do { \
									if ((x) > (UWord) SPM_SIZE * 65536) \
										AP_ASSERT(0, "Address overflow"); \
									} while (0);

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
	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	CHECK_ADDR_OVERFLOW(addr);
	#endif
	
	UWord i = addr >> 16;
	#if ADDR_MULTIPLE == 1
	UWord j = (addr & 0xffff);
	#elif ADDR_MULTIPLE == 4
	UWord j = (addr & 0xffff) / 4;
	#endif
	if (suf->table[i] == NULL) {
		
		suf->table[i] = VG_(calloc)("suf sm", sizeof(SSM), 1);
		AP_ASSERT(suf->table[i] != NULL, "SUF sm not allocable");
		
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
	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	CHECK_ADDR_OVERFLOW(addr);
	#endif
	
	UWord i = addr >> 16;
	if (suf->table[i] == NULL) return 0;
	
	#if ADDR_MULTIPLE == 1
	UWord j = (addr & 0xffff);
	#elif ADDR_MULTIPLE == 4
	UWord j = (addr & 0xffff) / 4;
	#endif
	return suf->table[i]->table[j];
}
