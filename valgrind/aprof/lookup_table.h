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
 * Last changed: $Date: 2013-09-06 00:55:22 +0200 (ven, 06 set 2013) $
 * Revision:     $Rev: 891 $
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


#define COMPRESS_DEBUG 0

//#define __i386__

#ifdef __i386__
#define LK_SIZE 65536 // 4GB
#else

#define LK_SIZE  65536 // 2048GB
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
    
    // cache
    UInt * last_chunk;
    UWord  last_chunk_index;
    
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
 * Compress all shadow memories. We re-assign all
 * the timestamps in order to compress the valid ts range.
 * 
 * array:  list of valid timestamps
 * size:   size of the previous array
 * memsha: all the thread shadow memories
 */
#if COMPRESS_DEBUG
void LK_compress(UInt * array, UInt size, LookupTable ** memsha, void * f);
#else
void LK_compress(UInt * array, UInt size, LookupTable ** memsha);
#endif
void LK_compress_rms(LookupTable * uf, UInt * arr_rid, UInt size_arr);

#endif
