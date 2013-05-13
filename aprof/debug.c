/*
 * Some debug functions
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

#if DEBUG_ALLOCATION

static UInt alloc_type_size[A_NONE] = {

    sizeof(BB), /* BB */
    sizeof(RoutineInfo), /* Routine */
    sizeof(Function), /* Function */ 
    sizeof(ThreadData),  /* thread */  
    NAME_SIZE, /* Function name */ 
    sizeof(Activation), /* Activation */ 
    NAME_SIZE, /* Object name */ 
    0, /* Pool page */
    sizeof(HashNode), /* ht node */
    
    #ifndef __i386__
    sizeof(ILT), /* intermediate table LK */
    #else
    0,
    #endif
    
    16384 * 4, /* LK segment (memory resol. 4) */
    sizeof(RMSInfo), /* Read memory size*/
    sizeof(HashTable), /* Hash Table */
    sizeof(void *), /* element array ht */
    sizeof(CCT), /* CCT */
    sizeof(Object), /* Object */ 
    NAME_SIZE /* Mangled name */

};

static char * alloc_type_name[A_NONE] = {
    "BasicBlock", "Routine", "Function", 
    "ThreadData", "FunctionName", "Activation",
    "ObjectName", "PoolPage", "HashNode", "IntermediateTableLK",
    "SegmentLK",  "RMSInfo",  "HashTable", "HT chain",
    "CCT", "Object", "MangledName"
};


static UInt alloc_counter[A_NONE] = { 0 };

void APROF_(add_alloc)(UWord type) {
    
    alloc_counter[type]++;
    /*
    if (alloc_counter[type] % 1000 == 0) {
        VG_(printf)("Allocated %u %s\n", 
            alloc_counter[type], alloc_type_name[type]); 
    }
    */
    
    return;
    
}

void APROF_(remove_alloc)(UWord type) {
    
    alloc_counter[type]--;
    return;
    
}

void APROF_(print_alloc)(void) {
    
    UInt est = 0;
    
    VG_(umsg)("Report allocations of aprof:\n\n");
    
    UInt i = 0;
    for (i = 0; i < A_NONE; i++) {
        VG_(umsg)("Allocated %u %s ~ %u kb (%u mb)\n", 
            alloc_counter[i], alloc_type_name[i],
            (alloc_counter[i] * alloc_type_size[i]) / 1024,
            (alloc_counter[i] * alloc_type_size[i]) / 1024 / 1024);
        est += alloc_counter[i] * alloc_type_size[i];
    }
    
    VG_(umsg)("\n");
    VG_(umsg)("Estimated space usage: %u kb (%u mb)\n\n", est/1024, est/1024/1024);
}

#endif
