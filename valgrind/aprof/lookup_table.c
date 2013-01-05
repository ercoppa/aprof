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
                            Irene Finocchi,
                            Romolo Marotta

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

/* 
 * # timestamps in the last level of the lookup table
 * default value for memory resolution = 4 => 65536/4
 */
static UInt APROF_(flt_size) = 16384;
                                      
/*
 * # of >> shifts in order to achieve the chosen resolution
 * default value for memory resolution = 4 => 2 shifts
 */
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
    
    UWord k = (addr >> 16) & 0x3fff;
    if (suf->table[i] == NULL) return 0;
    UInt * ssm = suf->table[i]->table[k];
    if (ssm == NULL) return 0;
    return ssm[j];
    
    #endif
    
    return 0;
}

void LK_compress(UInt * array, UInt size, LookupTable ** shamem) {
    
    UInt count_thread = APROF_(running_threads);
    UInt i = 0;
    UInt j = 0;
    UInt k = 0;
    UInt ts = 0;
    UInt t = 0;
    
    // scan global shadow memory (GSM)
    for(i = 0; i < LK_SIZE; i++){
    
        // chunk: a list of ts (e.g. ts for a 64KB segment)
        UInt * global_chunk = (UInt *) APROF_(global_shadow_memory)->table[i];

        #ifndef __i386__ // 64bit: 3 level lookup table...
        for(j = 0; j < ILT_SIZE; j++){

            /* 
             * check if this first level chunck of GSM is valid
             * if not we assume wts[x] = 0 
             */

            if(global_chunk != NULL)
                global_chunk = ((ILT *) global_chunk)->table[j];
        #endif

            /*
             * if global_chunk is invalid we expect also that 
             * threads chunks are invalid...
             */
            if(global_chunk == NULL) continue;

            for (k = 0; k < APROF_(flt_size); k++){
                
                ts = binary_search(array, 0, size, global_chunk[k]);
                
                // for each thread, access the relative chunk
                for(t = 0; t < count_thread; t++){
                    
                    // check if this cell was accessed by thread t
                    
                    UInt * local_chunk = (UInt *) shamem[t]->table[i];
                    if (local_chunk == NULL) continue;
                    
                    #ifndef __i386__ // 64bit: 3 level lookup table...
                    local_chunk = (UInt *) ((ILT*) local_chunk)->table[j];
                    if (local_chunk == NULL) continue;
                    #endif

                    if(local_chunk[k] < global_chunk[k]) {
                        
                        /* 
                         * it means that this value is not 
                         * accessed by thread t
                         */
                        local_chunk[k] = 0;

                    } else if(local_chunk[k] == global_chunk[k]) {
                    
                        /* thread t wrote this value */
                        local_chunk[k] = ts;

                    } else { 
                        
                        /* 
                         * thread t read this value so we have to reassign 
                         * the ts to the greater activation-ts that satisfy 
                         * wts[x] <= ats
                         */
                        local_chunk[k] = binary_search(array, ts, 
                                                    size, local_chunk[k]);
                    
                    }
                }
  
                global_chunk[k] = ts;

            }
        
        #ifndef __i386__    
        }
        #endif
        
    }
    
}


UInt binary_search(UInt * array, UInt init, UInt size, UInt ts){
    
    UInt l = init;
    UInt h = size - 1;
    UInt k;
    while(l != (h - 1)){

        k = (l + h) / 2;
        
        if(array[k] == ts)
            return k;
    
        else if(array[k] < ts)
            l = k;

        else
            h = k;
    
    }
    return l;

}
