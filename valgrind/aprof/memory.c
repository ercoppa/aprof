/*
 * Load, store and modify handlers
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

VG_REGPARM(3) void trace_access(UWord type, Addr addr, SizeT size) {
	
	ThreadData * tdata = current_tdata;
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Invalid tdata");
	#endif
	
	#if VERBOSE == 2
	if (type == LOAD) VG_(printf)("Load: %lu:%lu\n", addr, size);
	else if (type == STORE) VG_(printf)("Store: %lu:%lu\n", addr, size);
	else if (type == MODIFY) VG_(printf)("Modify: %lu:%lu\n", addr, size);
	#endif
	
	#if EVENTCOUNT == 1 || EVENTCOUNT == 3
	if (type == LOAD) tdata->num_read++;
	else if (type == STORE) tdata->num_write++;
	else if (type == MODIFY) tdata->num_modify++;
	#endif
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	if (tdata->stack_depth == 0) return;
	
	#if TRACE_FUNCTION
	if (tdata->skip) return;
	#endif
	
	#if COSTANT_MEM_ACCESS
	addr = addr & ~(ADDR_MULTIPLE-1);
	size = 1;
	#else
	
	#if ADDR_MULTIPLE > 1
	UWord diff = addr & (ADDR_MULTIPLE-1);
	addr -= diff;
	if (size + diff < ADDR_MULTIPLE) 
		size = 1;
	else if (((size + diff) % ADDR_MULTIPLE) == 0)
		size = (size + diff) / ADDR_MULTIPLE;
	else
		size = 1 + ((size + diff) / ADDR_MULTIPLE);
	#endif
	
	#endif
	
	unsigned int i = 0;
	
	#if !COSTANT_MEM_ACCESS
	for (i = 0; i < size; i++) {
	#endif
		
		#if SUF == 1

		int addr_depth = UF_insert(tdata->accesses, addr+(i*ADDR_MULTIPLE), tdata->stack_depth);
		
		if (tdata->stack_depth > addr_depth) {
			
			if (type == LOAD || type == MODIFY) {
				get_activation(tdata, tdata->stack_depth)->sms++;
				if (addr_depth >= 0)
					get_activation(tdata, addr_depth)->sms--;
			}
			
		}
		
		#else
		UWord old_aid = SUF_insert( tdata->accesses, 
									addr+(i*ADDR_MULTIPLE), 
									tdata->curr_aid);
		
		if (old_aid < tdata->curr_aid && (type == LOAD || type == MODIFY)) {
			get_activation(tdata, tdata->stack_depth)->sms++;
			if (old_aid > 0 && old_aid >= get_activation(tdata, 1)->aid) 
				get_activation_by_aid(tdata, old_aid)->sms--;

		}
		#endif
		
	#if !COSTANT_MEM_ACCESS
	}
	#endif
	
}
