/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Load, store and modify handlers
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

/* Memory resolution: we can aggregate addresses in order
 * to decrese the shadow memory. 
 * - 1 => finest resolution, each byte has its timestamp
 * - 2 => every 2 bytes we have a single timestamp 
 * - ...
 */
UInt APROF_(addr_multiple) = 4;

VG_REGPARM(3) void APROF_(trace_access)(UWord type, Addr addr, SizeT size) {
	
	ThreadData * tdata = APROF_(current_tdata);
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Invalid tdata");
	#endif
	
	#if VERBOSE > 1
	if (type == LOAD) VG_(printf)("Load: %lu:%lu\n", addr, size);
	else if (type == STORE) VG_(printf)("Store: %lu:%lu\n", addr, size);
	else if (type == MODIFY) VG_(printf)("Modify: %lu:%lu\n", addr, size);
	#endif
	
	#if EVENTCOUNT == 1 || EVENTCOUNT == 3
	if (type == LOAD) tdata->num_read++;
	else if (type == STORE) tdata->num_write++;
	else if (type == MODIFY) tdata->num_modify++;
	else AP_ASSERT(0, "Invalid type")
	#endif
	
	#if EMPTY_ANALYSIS
	return;
	#endif
    
	if (tdata->stack_depth == 0) return;
    
	#if TRACE_FUNCTION
	if (tdata->skip) return;
	#endif
	
	#if COSTANT_MEM_ACCESS
	addr = addr & ~(APROF_(addr_multiple)-1);
	//size = 1;
	#else
	
	if (APROF_(addr_multiple) > 1) {
		
		UInt diff = addr & (APROF_(addr_multiple)-1);
		addr -= diff;
		if (size + diff < APROF_(addr_multiple)) 
			size = 1;
		else if (((size + diff) % APROF_(addr_multiple)) == 0)
			size = (size + diff) / APROF_(addr_multiple);
		else
			size = 1 + ((size + diff) / APROF_(addr_multiple));
	
	}
	
	#endif
    
	#if !COSTANT_MEM_ACCESS
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
	#endif
		
		Activation * act = APROF_(get_activation)(tdata, tdata->stack_depth);
        
        UInt old_aid = LK_insert( tdata->accesses,
									
									#if !COSTANT_MEM_ACCESS
									addr+(i*APROF_(addr_multiple)),
									#else
									addr,
									#endif
									
									act->aid);
        
		if (old_aid < act->aid && (type == LOAD || type == MODIFY)) {
			
			#if 0
			if (inside_strcmp == 1) {
				VG_(printf)("Instruction: %#lx : %#lx\n", last_addr, addr);
			}
			#endif
			
			act->rms++;
			//VG_(printf)("Incremented RMS\n");
			if (old_aid > 0 && old_aid >= APROF_(get_activation)(tdata, 1)->aid) {
				
				APROF_(get_activation_by_aid)(tdata, old_aid)->rms--;
				//VG_(printf)("Decremented SMS of ancestor %s\n", 
				//	APROF_(get_activation_by_aid)(tdata, old_aid)->rtn_info->fn->name);
			
			}

		}
		
	#if !COSTANT_MEM_ACCESS
	}
	#endif
    
}
