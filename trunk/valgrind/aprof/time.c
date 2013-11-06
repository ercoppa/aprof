/*
 * time() function and instruction (VEX or Intel) counters
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */
 
 /*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2013, Emilio Coppa (ercoppa@gmail.com),
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

#if TIME != BB_COUNT
ULong APROF_(time)(ThreadData * tdata) {

    #if EMPTY_ANALYSIS
    return 0;
    #endif

    #if TIME == INSTR
    return tdata->instr;

    #elif TIME == RDTSC
    
    ULong ret;
    unsigned long lo, hi;
    asm( "rdtsc" : "=a" (lo), "=d" (hi) ); 
    return( lo | (hi << 32) );

//    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (ret));
//    __asm__ __volatile__("rdtsc": "=A" (ret));
    return ret;

    #elif TIME == BB_COUNT
    return tdata->bb_c;
    #endif

    return 0;

}
#endif

#if TIME == INSTR
VG_REGPARM(0) void APROF_(add_one_guest_instr)(void) {
    APROF_(current_tdata)->instr++;
}
#endif

#if TIME == BB_COUNT && !TRACE_FUNCTION
VG_REGPARM(0) void APROF_(add_one_guest_BB)(void) {
    APROF_(current_tdata)->bb_c++;
}
#endif

