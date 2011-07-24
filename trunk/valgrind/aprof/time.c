/*
 * Time() function and instruction (VEX or Intel) counters
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

UWord64 ap_time(void) {
	
	#if !TRACER && TIME == INSTR
	
	#if EMPTY_ANALYSIS
	return 0;
	#else
	return get_thread_data(0)->instr;
	#endif
	
	#elif !TRACER && TIME == RDTSC
	UWord64 ret;
	__asm__ __volatile__("rdtsc": "=A" (ret));
	return ret;
	#elif !TRACER && TIME == BB_COUNT
	return get_thread_data(0)->bb_c;
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
static UWord64 counter_instr = 0;
#endif

VG_REGPARM(0) void add_one_guest_instr(void) {
	#if EMPTY_ANALYSIS
	counter_instr++;
	#else
	get_thread_data(0)->instr++;
	#endif
}
#endif

#if EVENTCOUNT
UWord bb_c = 0;
#endif

#if EVENTCOUNT == 0 || EVENTCOUNT >= 2

#if TIME == BB_COUNT

VG_REGPARM(0) void add_one_guest_BB(void) {

	#if EVENTCOUNT == 0
	get_thread_data(0)->bb_c++;
	#else
	bb_c++;
	#endif
}
#endif

#endif

