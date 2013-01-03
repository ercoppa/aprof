/*
 * Lookup table header
 * 
 * This is a very simple implementation of a lookup table. We use it
 * for shadowing the accessed memory cells with timestamps. Each
 * timestamp is a 32bit counter containing the value of the last
 * activation of a function that has accessed the memory cell.
 * 
 * In order to decrease the size of the shadow memory, we
 * can "aggregate" one or more (ADDR_MULTIPLE) addresses in a single
 * timestamp.
 * 
 * On:
 * - 32 bit => 2 level lookuptable => 4GB covered
 * - 64 bit => 3 level lookuptable => 2048GB covered
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

#ifndef _LK_H_
#define _LK_H_

//#define __i386__

#ifdef __i386__
#define LK_SIZE 65536 // 4GB
#else

#define LK_SIZE  2048 // 2048GB
#define ILT_SIZE 16384 // 1GB

/* 
 * intermediate level table (only 64bit machines)
 */
typedef struct ILT {
    UInt * table[ILT_SIZE];
} ILT;

#endif

/*
 * First level of the lookup table
 */
typedef struct LookupTable {
    #ifdef __i386__
    UInt * table[LK_SIZE];
    #else
    ILT * table[LK_SIZE];
    #endif
} LookupTable;

/*
 * create the lookup table
 */
LookupTable * LK_create(void);

/*
 * destroy the lookup table
 */
void LK_destroy(LookupTable * lt);

/*
 * Insert a value (timestamp) for a specific key (address).
 * Return the old value for that key.
 */
UInt LK_insert(LookupTable * lt, UWord key, UInt value);

/*
 * Get current value (timestamp) for a key (address)
 */
UInt LK_lookup(LookupTable * lt, UWord key);

/* 
 * Re-assign timestamps of global shadow memory (compress
 * valid range).
 * 
 * After some time, the timestamp counter can overflow. So we
 * "compress" the lookup table: 
 * 
 *      e.g. if our shadow memory contains:
 *           
 *              (addrA, 18) (addrB, 20) (addrC, 40) (addrC, 25)
 * 
 *           and our shadow stack contains activations with the
 *           the following timestamps:
 * 
 *               15, 19, 39
 * 
 *           we replace the timestamps in this way:
 *              
 *              (addrA, 1) (addrB, 2) (addrC, 3) (addrC, 2)
 * 
 *           An old timestamp is replaced with the the position (in the 
 *           array) of the bigger timestamp that is equal or smaller  
 * 
 * arr_rid contains the valid timestamps for the current shadow stack
 */
void LK_compress_global(LookupTable * lt, UInt * arr_rid, UInt size_arr);

/*
 * Compress all local (thread) shadow memories. We re-assign all
 * the timestamps in order to compress the valid ts range.
 * 
 * array:  list of valid timestamps
 * size:   size of the previous array
 * memsha: all the thread shadow memories
 */
void LK_compress_all_local(UInt * array, UInt size, LookupTable ** memsha);

/*
 * Binary search in an array...
 */
UInt binary_search(UInt * array, UInt init, UInt size, UInt ts);

#endif
