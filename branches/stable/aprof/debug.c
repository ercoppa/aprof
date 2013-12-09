/*
 * Some debug functions
 * 
 * Last changed: $Date: 2013-02-28 15:23:24 +0100 (gio, 28 feb 2013) $
 * Revision:     $Rev: 811 $
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2014, Emilio Coppa (ercoppa@gmail.com),
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

    sizeof(BB),             // BB 
    sizeof(RoutineInfo),    // Routine
    sizeof(Function),       // Function
    sizeof(ThreadData),     // thread
    NAME_SIZE,              // Function name
    sizeof(Activation),     // Activation
    NAME_SIZE,              // Object name
    0,                      // Pool page
    sizeof(HashNode),       // ht node

#ifndef __i386__
    sizeof(ILT),            // intermediate LKT
#else
    0,
#endif
    
    16384 * 4,              // LK segment (memory resol. 4)
    sizeof(LookupTable),    // LK
    sizeof(Input),          // input tuple
    sizeof(HashTable),      // HT
    sizeof(void *),         // element array ht
    sizeof(CCTNode),        // CCT node
    sizeof(Object),         // Object
    NAME_SIZE,              // Mangled name
    sizeof(FILE)            // FILE
};

static const HChar * alloc_type_name[A_NONE] = {
    "BasicBlock", "Routine", "Function",
    "ThreadData", "FunctionName", "Activation",
    "ObjectName", "PoolPage", "HashNode", "IntermediateTableLK",
    "SegmentLK", "LK", "Input", "HashTable", "HT chain",
    "CCT", "Object", "MangledName", "FILE"
};

static UInt alloc_counter[A_NONE] = {0};

void APROF_(add_alloc)(UWord type) {
    alloc_counter[type]++;
}

void APROF_(remove_alloc)(UWord type) {
    alloc_counter[type]--;
    return;
}

void APROF_(print_alloc)(void) {

    //VG_(umsg)("Report allocations of aprof:\n\n");

    UInt est = 0;
    UInt i = 0;
    for (i = 0; i < A_NONE; i++) {
        
        if (alloc_counter[i] > 0)
            VG_(umsg)("Allocated %u %s ~ %u kb (%u mb)\n",
                    alloc_counter[i], alloc_type_name[i],
                    (alloc_counter[i] * alloc_type_size[i]) / 1024,
                    (alloc_counter[i] * alloc_type_size[i]) / 1024 / 1024);
        
        est += alloc_counter[i] * alloc_type_size[i];
    }

    if (est > 0)
        VG_(umsg)("\nEstimated space usage: %u kb (%u mb)\n\n", 
                            est/1024, est/1024/1024);
}

#endif
