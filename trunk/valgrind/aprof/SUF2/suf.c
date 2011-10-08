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

	UInt i = 0;
	while (i < SPM_SIZE) {
		if (suf->table[i] != NULL) 
			VG_(free)(suf->table[i]);
		i++;
	}
	VG_(free)(suf);
}

UInt SUF_insert(StackUF * suf, UWord addr, UInt rid)
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
		
		#if DEBUG_ALLOCATION
		add_alloc(SEG_SUF);
		#endif
		
		suf->table[i]->table[j] = rid;
		return 0;
		
	}
	
	UInt old = suf->table[i]->table[j];
	if (old < rid) /* avoid a write if possible... */
		suf->table[i]->table[j] = rid;
	return old;

}

UInt SUF_lookup(StackUF * suf, UWord addr)
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

void SUF_compress(StackUF * uf, UInt * arr_rid, UInt size_arr) {
	
	UInt i = 0;
	while (i < SPM_SIZE) {
		SSM * table = uf->table[i];
		if (table != NULL) {
			UInt j = 0;
			UInt rid = 0;
			for (j = 0; j < SSM_SIZE; j++){
				
				rid = table->table[j];
				int k = 0;
				for (k = size_arr - 1; k >= 0; k--) {
					
					if (arr_rid[k] <= rid) {
						table->table[j] = k + 1;
						break;
					}
					
				}
				
				AP_ASSERT(k >= 0, "Invalid reassignment");
				
			}
		}
		i++;
	}

}
