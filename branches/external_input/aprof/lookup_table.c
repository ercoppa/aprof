/*
 * lookup table implementation
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "aprof.h"

// # timestamp in the last level of the lookup table
static UInt APROF_(flt_size) = 16384; // default value for 
                                      // memory resolution = 4
                                      
static UInt APROF_(res_shift) = 2;
                                                                                 
LookupTable * LK_create(void) {

	switch(APROF_(addr_multiple)) {
		
		case 4:
			APROF_(flt_size) = 16384; 
			APROF_(res_shift) = 2;
			break;
		
		case 1:
			APROF_(flt_size) = 65536; 
			APROF_(res_shift) = 0;
			break;
		
		case 2:
			APROF_(flt_size) = 32768; 
			APROF_(res_shift) = 1;
			break;
		
		case 8:
			APROF_(flt_size) = 8192; 
			APROF_(res_shift) = 3;
			break;
		
		case 16:
			APROF_(flt_size) = 4096; 
			APROF_(res_shift) = 4;
			break;
		
		default:
			AP_ASSERT(0, "Supported memory resolutions: 1, 2, 4, 8 or 16");
	}

	return VG_(calloc)("Lookuptable", sizeof(struct LookupTable), 1);

}

void LK_destroy(LookupTable * lk) {

	UInt i = 0;
	while (i < LK_SIZE) {
		
		if (lk->table[i] != NULL) {
		
			#ifndef __i386__
			UInt j = 0;
			while (j < ILT_SIZE) {
				
				if (lk->table[i]->table[j] != NULL)
					VG_(free)(lk->table[i]->table[j]);
			
				j++;
			
			}
			#endif
			
			VG_(free)(lk->table[i]);
		}
		i++;
	}
	VG_(free)(lk);
	
}

UInt LK_insert(LookupTable * suf, UWord addr, UInt ts) {
	
	#ifdef __i386__
	UWord i = addr >> 16;
	#else
	
    //VG_(printf)("Address is %llu\n", (UWord)addr);
    
	UWord i = addr >> 30; // 14 + 16
	
    //VG_(printf)("index i %llu\n", i);
    
	#if CHECK_OVERFLOW
	AP_ASSERT((i < LK_SIZE), "Address overflow");
	#endif
	
	UWord k = (addr >> 16) & 0x3fff;
	
    //VG_(printf)("index k %llu\n", k);
    
	#endif
	
	// this is slow!!! 
	//UWord j = (addr & 0xffff) / APROF_(addr_multiple);
	// faster:
	UWord j = (addr & 0xffff) >> APROF_(res_shift);
	
	#ifndef __i386__
	if (suf->table[i] == NULL) {
		
		suf->table[i] = VG_(calloc)("suf sm", sizeof(ILT), 1);
		#if DEBUG
		AP_ASSERT(suf->table[i] != NULL, "SUF sm not allocable");
		#endif
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(SEG_SUF);
		#endif
		
	}
	#endif

	#ifdef __i386__
	if (suf->table[i] == NULL) {
	#else
	if (suf->table[i]->table[k] == NULL) {
	#endif
	
		#ifdef __i386__
		suf->table[i] = VG_(calloc)("suf sm", sizeof(UInt) * APROF_(flt_size), 1);
		#else
		suf->table[i]->table[k] = VG_(calloc)("suf sm", sizeof(UInt) * APROF_(flt_size), 1);
		#endif
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(SEG_SUF);
		#endif
		
		#ifdef __i386__
		suf->table[i][j] = ts;;
		#else
		suf->table[i]->table[k][j] = ts;
		#endif

		return 0;
	
	}
	
	#ifdef __i386__
	UInt old = suf->table[i][j];
	#else
	UInt old = suf->table[i]->table[k][j];
	#endif
	if (old < ts) /* avoid a write if possible... */
	#ifdef __i386__
		suf->table[i][j] = ts;
	#else
		suf->table[i]->table[k][j] = ts;
	#endif
	
	return old;

}

UInt LK_lookup(LookupTable * suf, UWord addr) {
	
	UWord j = (addr & 0xffff) >> APROF_(res_shift);
	
	#ifdef __i386__
	
	UWord i = addr >> 16;
	UInt * ssm = suf->table[i];
	if (ssm == NULL) return 0;
	return ssm[j];
	
	#else
	
	UWord i = addr >> 30; // 14 + 16
	
	#if CHECK_OVERFLOW
	AP_ASSERT((i <= LK_SIZE), "Address overflow");
	#endif
	
	if (suf->table[i] == NULL) return 0;
	UWord k = (addr >> 16) & 0x3fff;
	UInt * ssm = suf->table[i]->table[k];
	if (ssm == NULL) return 0;
	return ssm[j];
	
	#endif
	
	return 0;
}

void LK_compress(LookupTable * uf, UInt * arr_rid, UInt size_arr) {
	
	//int q = 0;
	//for (q = 0; q < size_arr; q++) VG_(printf)("arr_rid[%d]: %u\n", q, arr_rid[q]);
	
	UInt i = 0;
	while (i < LK_SIZE) {
		
		#ifndef __i386__
		UInt q = 0;
		
		if (uf->table[i] != NULL) {
		
			while (q < ILT_SIZE) {
				
				UInt * table = uf->table[i]->table[q]; 
		#else
				UInt * table = uf->table[i];
		#endif
				
				if (table != NULL) {

					UInt j = 0;
					UInt rid = 0;
					for (j = 0; j < APROF_(flt_size); j++){
						
						rid = table[j];
						if (rid == 0) continue;
						int k = 0;
						for (k = size_arr - 1; k >= 0; k--) {
							
							if (arr_rid[k] <= rid && arr_rid[k]!=0) {

								#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
								counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[j]);
								counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", k);
								if(counter_pre>16384-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<16384/4) ((int*)buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>16384-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<16384/4) ((int*)buffer_post)[z++] =0;
									counter_post = 0;
								}
								#endif

								table[j] = k;
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
						if (k < 0)
								

							table[j] = 0;
				
						
						//AP_ASSERT(k >= 0, "Invalid reassignment");
						
					}
					//VG_(printf)("Scanned %u bucket\n", j);
			
				}
		#ifndef __i386__
			q++;
			}
		}
		#endif
		i++;
	}

}

void LK_compress_all_shadow(UInt * array, UInt dim, LookupTable** shamem_array){
	
	/*#if OVERFLOW_DEBUG != 0
	VG_(printf)("\nCOMPRESS GSM\n");
	#endif
	
	#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
	char* buffer_pre =  VG_(calloc)("overflow reports file buffer1", 16384, sizeof(char));
	char* buffer_post =  VG_(calloc)("overflow reports file buffer2", 16384, sizeof(char));
	UInt counter_pre = 0;
	UInt counter_post = 0;
	extern FILE* pre_overflow;
	extern FILE* post_overflow;  
	counter_pre += VG_(sprintf)(buffer_pre, "\nGLOBAL SHADOW MEMORY\n\n");
	counter_post += VG_(sprintf)(buffer_post, "\nGLOBAL SHADOW MEMORY\n\n");
	#endif
		#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
							counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
							counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", 0);
							if(counter_pre>16384-11){
								APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
								int z = 0;
								while(z<16384/4) ((int*)buffer_pre)[z++] =0;
								counter_pre = 0;
							}
							if(counter_post>16384-11){
								APROF_(fwrite)(post_overflow, buffer_post, counter_post);
								int z = 0;
								while(z<16384/4) ((int*)buffer_post)[z++] =0;
								counter_post = 0;
							}
							#endif*/

	UInt count_thread = APROF_(running_threads);
	


	UInt i, j, k,ts, t;
	ts = i = j = k = t =0;

	
	/* Compress all shadow memory */
	
	for(i = 0; i < LK_SIZE; i++){
		
		/*Scan GSM*/

		UInt* table = (UInt*) APROF_(global_shadow_memory)->table[i];

			for(j = 0; j < ILT_SIZE; j++){

				/*check if this first level chunck of GSM is valid
				* if not we assume wts[x] = 0 */

				if(table != NULL)
					table = ((ILT*) table)->table[j];

					for (k = 0; k < APROF_(flt_size); k++){
						
						/* check if this last level of GSM is valid
						* if not we assume wts[x] = 0 */

						if(table != NULL)	
							ts = binary_search(array, 0, dim, table[k]);
						else
							ts = 0;

						for(t = 0; t < count_thread; t++){
							UInt* app_tab = shamem_array[t]->table[i];

							/* check if this cell was accessed by thread t*/

							if(app_tab == NULL || 
									(app_tab = ((ILT*) app_tab)->table[j]) == NULL) continue;
							
							/* it means that this value is not accessed by thread t*/
							if(app_tab[k] < table[k])
									app_tab[k] = 0;

							/* thread t wrote this value*/
							else if(app_tab[k] == table[k])
									app_tab[k] = ts;

							/* thread t read this value so we have to reassign the ts
							* to the greater activation-ts that satisfy  wts[x] <= ats*/

							else 
								app_tab[k] = binary_search(array, ts, dim, app_tab[k]);
		
						}

						if(table != NULL)	
							table[k] = ts;

					}
				
			}
		
	}
}

UInt binary_search(UInt* array, UInt init, UInt dim, UInt ts){
	UInt l,h,k;
	l = init;
	h = dim -1;
	while(l != h-1){

						k = (l+h)/2 ;
						
						if(array[k] == ts)
							return k;
					
						else if(array[k] < ts)
							l = k;

						else
							h = k;
	
	}
	return l;


}
