/*
 * Time() function and instruction (VEX or Intel) counters
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

ULong ap_time(void) {
	
	#if !TRACER && TIME == INSTR
	
	#if EMPTY_ANALYSIS
	return 0;
	#else
	return get_thread_data(0)->instr;
	#endif
	
	#elif !TRACER && TIME == RDTSC
	ULong ret;
	__asm__ __volatile__("rdtsc": "=A" (ret));
	return ret;
	#elif !TRACER && TIME == BB_COUNT
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

#if EMPTY_ANALYSIS
static ULong counter_instr = 0;
#endif

VG_REGPARM(0) void add_one_guest_instr(void) {
	#if EMPTY_ANALYSIS
	counter_instr++;
	#else
	current_tdata->instr++;
	#endif
}
#endif

#if EVENTCOUNT
UWord bb_c = 0;
#endif

#if EVENTCOUNT == 0 || EVENTCOUNT >= 2

#if TIME == BB_COUNT && !TRACE_FUNCTION

VG_REGPARM(0) void add_one_guest_BB(void) {

	#if EVENTCOUNT == 0
	current_tdata->bb_c++;
	#else
	bb_c++;
	#endif
}
#endif

#endif

