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
                                      // memory resoloution = 4
                                                                                      
LookupTable * LK_create(void) {

	switch(APROF_(addr_multiple)) {
		
		case 4:
			APROF_(flt_size) = 16384; break;
		
		case 1:
			APROF_(flt_size) = 65536; break;
		
		case 2:
			APROF_(flt_size) = 32768; break;
		
		case 8:
			APROF_(flt_size) = 8192; break;
		
		case 16:
			APROF_(flt_size) = 4096; break;
		
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
	
	UWord j = (addr & 0xffff) / APROF_(addr_multiple);
	
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
	UInt * ssm = suf->table[i];
	#else
	UInt * ssm = suf->table[i]->table[k];
	#endif
	
	if (ssm == NULL) {
		
		#ifdef __i386__
		suf->table[i] = VG_(calloc)("suf sm", sizeof(UInt) * APROF_(flt_size), 1);
		ssm = suf->table[i];
		#else
		suf->table[i]->table[k] = VG_(calloc)("suf sm", sizeof(UInt) * APROF_(flt_size), 1);
		ssm = suf->table[i]->table[k];
		#endif
		
		#if DEBUG
		AP_ASSERT(ssm != NULL, "SUF sm not allocable");
		#endif
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(SEG_SUF);
		#endif
	
	}
	
	UInt old = ssm[j];
	if (old < ts) /* avoid a write if possible... */
		ssm[j] = ts;
    
	return old;

}

UInt LK_lookup(LookupTable * suf, UWord addr) {
	
	UWord j = (addr & 0xffff) / APROF_(addr_multiple);
	
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
	
	UWord k = (addr >> 16) & 0x4000;
	if (suf->table[i] == NULL) return 0;
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
							
							if (arr_rid[k] <= rid) {
								table[j] = k + 1;
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
						if (k < 0) table[j] = 0;
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

}
