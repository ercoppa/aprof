/*
 * Time() function and instruction (VEX or Intel) counters
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

ULong ap_time(void) {
	
	#if EMPTY_ANALYSIS
	return 0;
	#endif
	
	#if TIME == INSTR
	return get_thread_data(0)->instr;
	
	#elif TIME == RDTSC
	ULong ret;
	__asm__ __volatile__("rdtsc": "=A" (ret));
	return ret;
	
	#elif TIME == BB_COUNT
	return current_tdata->bb_c;
	#endif
	
	return 0;

}

/*
#if COUNT_VEX_I
static VG_REGPARM(0) void add_one_IRStmt(void) {
   vex_i++;
}
#endif
*/

#if TIME == INSTR
VG_REGPARM(0) void add_one_guest_instr(void) {
	current_tdata->instr++;
}
#endif

#if TIME == BB_COUNT && !TRACE_FUNCTION
VG_REGPARM(0) void add_one_guest_BB(void) {
	current_tdata->bb_c++;
}
#endif

