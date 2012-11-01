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
	#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
	char* buffer_pre =  VG_(calloc)("overflow reports file buffer3", BUFFER_SIZE, sizeof(char));
	char* buffer_post =  VG_(calloc)("overflow reports file buffer4", BUFFER_SIZE, sizeof(char));
	UInt counter_pre = 0;
	UInt counter_post = 0;
	extern FILE* pre_overflow;
	extern FILE* post_overflow;  
	counter_pre += VG_(sprintf)(buffer_pre, "\n SHADOW MEMORY PRIVATA\n\n");
	counter_post += VG_(sprintf)(buffer_post, "\n SHADOW MEMORY PRIVATA\n\n");
	#endif
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
								if(counter_pre>BUFFER_SIZE-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<counter_pre) (buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>BUFFER_SIZE-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<counter_post) (buffer_post)[z++] =0;
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
						if (k < 0){
								#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
								counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[j]);
								counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", 0);
								if(counter_pre>BUFFER_SIZE-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<counter_pre) (buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>BUFFER_SIZE-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<counter_post) (buffer_post)[z++] =0;
									counter_post = 0;
								}
								#endif

							table[j] = 0;
				
						}
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
	//VG_(printf)("Scanned %u segment\n", i);
	counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "\n$\n");
	counter_post += VG_(sprintf)(buffer_post+counter_post, "\n$\n");
	APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
	APROF_(fwrite)(post_overflow, buffer_post, counter_post);
	VG_(free)(buffer_pre);
	VG_(free)(buffer_post);
}

void LK_compress_global(UInt * array, UInt dim){
	
	#if OVERFLOW_DEBUG != 0
	VG_(printf)("\nCOMPRESS GSM\n");
	#endif
	
	#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
	char* buffer_pre =  VG_(calloc)("overflow reports file buffer1", BUFFER_SIZE, sizeof(char));
	char* buffer_post =  VG_(calloc)("overflow reports file buffer2", BUFFER_SIZE, sizeof(char));
	UInt counter_pre = 0;
	UInt counter_post = 0;
	extern FILE* pre_overflow;
	extern FILE* post_overflow;  
	counter_pre += VG_(sprintf)(buffer_pre, "\nGLOBAL SHADOW MEMORY\n\n");
	counter_post += VG_(sprintf)(buffer_post, "\nGLOBAL SHADOW MEMORY\n\n");
	#endif
	

	UInt i, j, k, h, l;
	k = i = j = l = 0;
	
	/* compress different write-timestamps between 
	* the same activation-timestamps */
	
	for(i = 0; i < LK_SIZE; i++){
		
		#ifndef __i386__
		
		if(APROF_(global_shadow_memory)->table[i] != NULL)
			
			for(j = 0; j < ILT_SIZE; j++){
				
				UInt* table = APROF_(global_shadow_memory)->table[i]->table[j];

		#else
			UInt* table = APROF_(global_shadow_memory)->table[i];
		#endif
		
			if(table != NULL){
				
				UInt c;
				for (c = 0; c < APROF_(flt_size); c++){
					if(table[c] == 0)continue;
					UInt ts = table[c];
					
					
					h = 1;
					l = dim - 2;
					
					/*hope there are a lot "ancient" writes*/

					if(ts < array[h]){

							#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
							counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
							counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", 0);
							if(counter_pre>BUFFER_SIZE-11){
								APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
								int z = 0;
								while(z<counter_pre) (buffer_pre)[z++] =0;
								counter_pre = 0;
							}
							if(counter_post>BUFFER_SIZE-11){
								APROF_(fwrite)(post_overflow, buffer_post, counter_post);
								int z = 0;
								while(z<counter_post) (buffer_post)[z++] =0;
								counter_post = 0;
							}
							#endif

						table[c] = 0;
							
							#if OVERFLOW_DEBUG == 1 || OVERFLOW_DEBUG == 3
								VG_(printf)("%u => %u\n", ts, 0);
							#endif

						continue;
					}
					
					/*vice versa*/

					if(ts > array[l]){
							
							#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
							counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
							counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", l+1);
							if(counter_pre>BUFFER_SIZE-11){
								APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
								int z = 0;
								while(z<counter_pre) (buffer_pre)[z++] =0;
								counter_pre = 0;
							}
							if(counter_post>BUFFER_SIZE-11){
								APROF_(fwrite)(post_overflow, buffer_post, counter_post);
								int z = 0;
								while(z<counter_post) (buffer_post)[z++] =0;
								counter_post = 0;
							}
							#endif

						table[c] = ++l;
						if(array[l] == 0 || ts < array[l])
									array[l] = ts;
						
					
						#if OVERFLOW_DEBUG == 1 || OVERFLOW_DEBUG == 3
								VG_(printf)("%u => %u\n", ts, l);
						#endif
						continue;
					}

					

					/* binary search */
					
					while(h != l-2){

						k = (l+h)/2 ;
						if(k % 2 == 0) k--;
						

						//VG_(printf)("h: %u, k:%u, l:%u\n", h, k, l);
						if(array[k] < ts){ 
							h = k;
							
							/*
							 * Check if array[k] < ts < array[k + 2]
							 */

							if(ts < array[k + 2]){
								k++;
								/* update to older write */
								if(array[k] == 0 || ts < array[k])
									array[k] = ts;
								
								/* assign new write-ts */

								#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
								counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
								counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", k);
								if(counter_pre>BUFFER_SIZE-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<counter_pre) (buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>BUFFER_SIZE-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<counter_post) (buffer_post)[z++] =0;
									counter_post = 0;
								}
								#endif



								table[c] = k;
								break;
							}

						} else {
							
							
							l = k;

							/*
							 * Check if array[k - 2] < ts < array[k]
							 */

							if(ts > array[k-2]){

								k--;
								/* update to older write */
								if(array[k] == 0 || ts < array[k])
									array[k] = ts;
							
							
								/* assign new write-ts */

								#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
								counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
								counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", k);
								if(counter_pre>BUFFER_SIZE-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<counter_pre) (buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>BUFFER_SIZE-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<counter_post) (buffer_post)[z++] =0;
									counter_post = 0;
								}
								#endif

								table[c] = k;
								
								break;
							}
							
							
						}
						
					}
		
					/*base case
					*ex: h = 1 and l = 3 	
					*	the possible values are: 0 , 2	or 4*/
					
					if(h == l-2){
						
						if(ts < array[h])
							k = h-1;
						else if (ts > array[l])
							k = l+1;
						else //paranoia
							k = h+1;
						
						/* update to older write */
						if(array[k] == 0 || ts < array[k])
									array[k] = ts;
							

								#if OVERFLOW_DEBUG == 2 || OVERFLOW_DEBUG == 3
								counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "%u\n", table[c]);
								counter_post += VG_(sprintf)(buffer_post+counter_post, "%u\n", k);
								if(counter_pre>BUFFER_SIZE-11){
									APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
									int z = 0;
									while(z<counter_pre) (buffer_pre)[z++] =0;
									counter_pre = 0;
								}
								if(counter_post>BUFFER_SIZE-11){
									APROF_(fwrite)(post_overflow, buffer_post, counter_post);
									int z = 0;
									while(z<counter_post) (buffer_post)[z++] =0;
									counter_post = 0;
								}
								#endif							


								/* assign new write-ts */
								table[c] = k;

					
					}
#if OVERFLOW_DEBUG == 1 || OVERFLOW_DEBUG == 3
								VG_(printf)("%u => %u\n", ts, k);
#endif
				}
			}
		#ifndef __i386__
		}
		#endif
	}
	counter_pre += VG_(sprintf)(buffer_pre+counter_pre, "\n$\n");
	counter_post += VG_(sprintf)(buffer_post+counter_post, "\n$\n");
	APROF_(fwrite)(pre_overflow, buffer_pre, counter_pre);
	APROF_(fwrite)(post_overflow, buffer_post, counter_post);
	
	VG_(free)(buffer_pre);
	VG_(free)(buffer_post);
}
