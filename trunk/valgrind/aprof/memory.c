/*
 * Load, store and modify handlers
 * 
 * Last changed: $Date: 2011-07-08 00:14:45 +0200 (Fri, 08 Jul 2011) $
 * Revision:     $Rev: 70 $
 */

#include "aprof.h"

#if EVENTCOUNT
UWord read_n   = 0;
UWord write_n  = 0;
UWord modify_n = 0;
#endif

VG_REGPARM(3) void trace_access(UWord type, Addr addr, SizeT size) {
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	ThreadData * tdata = get_thread_data(0);
	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in trace_load");
	#endif
	
	/* We start tracing after main() */
	if (tdata->stack_depth == 0) return;
	
	#if TRACER
	char buf[24];
	VG_(sprintf)(buf, "a:%u:%u\n", size, ((unsigned int) addr));
	ap_fwrite(tdata->log, (void*)buf, VG_(strlen)(buf));
	return;
	#endif
	
	#if VERBOSE == 2
	if (type == LOAD) VG_(printf)("Load: %lu:%lu\n", addr, size);
	else if (type == STORE) VG_(printf)("Store: %lu:%lu\n", addr, size);
	else if (type == MODIFY) VG_(printf)("Modify: %lu:%lu\n", addr, size);
	#endif

	#if EVENTCOUNT
	if (type == LOAD) read_n++;
	else if (type == STORE) write_n++;
	else if (type == MODIFY) modify_n++;
	#endif
	
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
	
	unsigned int i = 0;
	for (i = 0; i < size; i++) {
		
		int addr_depth = UF_lookup(tdata->accesses, addr+(ADDR_MULTIPLE*i));
		
		if (tdata->stack_depth > addr_depth) {
			
			UF_insert(tdata->accesses, addr+(ADDR_MULTIPLE*i), tdata->stack_depth);
			
			if (type == LOAD || type == MODIFY) {
				get_activation(tdata, tdata->stack_depth)->sms++;
				if (addr_depth >= 0)
					get_activation(tdata, addr_depth)->sms--;
			}
			
		}
		
	}
	
}
