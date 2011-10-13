#include "aprof.h"

StackUF * SUF_create(void)
{
	StackUF * s = VG_(calloc)("suf pm", sizeof(struct SPM), 1);
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
	
	UWord i = addr >> 16;
	
	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	AP_ASSERT((i <= SPM_SIZE), "Address overflow");
	#endif
	
	#if ADDR_MULTIPLE == 1
	UWord j = (addr & 0xffff);
	#elif ADDR_MULTIPLE == 4
	UWord j = (addr & 0xffff) / 4;
	#endif
	if (suf->table[i] == NULL) {
		
		suf->table[i] = VG_(calloc)("suf sm", sizeof(SSM), 1);
		#if DEBUG
		AP_ASSERT(suf->table[i] != NULL, "SUF sm not allocable");
		#endif
		
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
	
	UWord i = addr >> 16;
	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	AP_ASSERT((i <= SPM_SIZE), "Address overflow");
	#endif
	
	if (suf->table[i] == NULL) return 0;
	
	#if ADDR_MULTIPLE == 1
	UWord j = (addr & 0xffff);
	#elif ADDR_MULTIPLE == 4
	UWord j = (addr & 0xffff) / 4;
	#endif
	return suf->table[i]->table[j];
}

void SUF_compress(StackUF * uf, UInt * arr_rid, UInt size_arr) {
	
	//int q = 0;
	//for (q = 0; q < size_arr; q++) VG_(printf)("arr_rid[%d]: %u\n", q, arr_rid[q]);
	
	UInt i = 0;
	while (i < SPM_SIZE) {
		
		SSM * table = uf->table[i];
		if (table != NULL) {
			UInt j = 0;
			UInt rid = 0;
			for (j = 0; j < SSM_SIZE; j++){
				
				rid = table->table[j];
				if (rid == 0) continue;
				int k = 0;
				for (k = size_arr - 1; k >= 0; k--) {
					
					if (arr_rid[k] <= rid) {
						table->table[j] = k + 1;
						//VG_(printf)("reassign [%u:%d] with %d\n", i, j, k+1);
						break;
					}
					
				}
				
				/* 
				 * This means that this address was accessed by 
				 * an activation no more in stack, and all its
				 * ancestors are dead (for example we are dealing
				 * with an aid of a setup libc function
				 * invoked before main() )
				 */
				if (k < 0) table->table[j] = 0;
				//AP_ASSERT(k >= 0, "Invalid reassignment");
				
			}
			//VG_(printf)("Scanned %u bucket\n", j);
		}
		i++;
	}
	//VG_(printf)("Scanned %u segment\n", i);

}

/*
void SUF_print(StackUF * uf) {
	
	UInt seg_p = 0;
	UInt i = 0;
	while (i < SPM_SIZE) {
		
		SSM * table = uf->table[i];
		
		if (table != NULL) {
			
			VG_(printf)("\nSegment %u\n", i);

			int j = 0;
			for (j = 0; j < SSM_SIZE; j++){
				
				if (table->table[j] > 0) {
					VG_(printf)("[%d] rid %u\n", j, table->table[j]);
					seg_p = 1;
				} 
			}
		}
		i++;
		if (seg_p > 0) return; 
	}
	
}
*/
