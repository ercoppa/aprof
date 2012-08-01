/*
 * Thread event handlers (start, exit, running)
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

/* All threads running */
static ThreadData * threads[VG_N_THREADS];

/* Current running thread ID */
ThreadId APROF_(current_TID) = VG_INVALID_THREADID; /* 0 */

/* Current running thread data */
ThreadData * APROF_(current_tdata) = NULL; 

static ThreadData * APROF_(thread_start)(ThreadId tid){

	#if VERBOSE
	VG_(printf)("start thread data %d\n", tid);
	#endif
	
	ThreadData * tdata = VG_(calloc)("thread_data", sizeof(ThreadData), 1);
	#if DEBUG
	AP_ASSERT(tdata != NULL, "thread data not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(TS);
	#endif
	
	#if DEBUG
	AP_ASSERT(tid - 1 < VG_N_THREADS && tid > 0, "Tid is too big");
	#endif
	threads[tid-1] = tdata;
	
	/* we deallocate all routine info when we generate the report */
	tdata->routine_hash_table = HT_construct(NULL);
	AP_ASSERT(tdata->routine_hash_table != NULL, "rtn ht not allocable");
	
	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(HT);
	#endif
	
	tdata->stack_depth = 0;
	tdata->max_stack_size = STACK_SIZE;
	tdata->stack = VG_(calloc)("stack", STACK_SIZE * sizeof(Activation), 1);
	#if DEBUG
	AP_ASSERT(tdata->stack != NULL, "stack not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	int j = 0;
	for (j = 0; j < STACK_SIZE; j++) APROF_(add_alloc)(ACT);
	#endif
	
	//tdata->next_routine_id = 0;
	
	#if TRACE_FUNCTION
	tdata->last_bb = NULL;
	#endif
	
	#if EMPTY_ANALYSIS
	return tdata;
	#endif
	
	tdata->accesses = LK_create();
	tdata->next_aid = 1;
	
	#if CCT
	
	// allocate dummy CCT root
	tdata->root = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
	#if DEBUG
	AP_ASSERT(tdata->root != NULL, "Can't allocate CCT root node");
	#endif

	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(CCTS);
	#endif

	//tdata->root->context_id = 0;
	
	tdata->next_context_id = 1;
	#endif
	
	#if TIME == RDTSC
	tdata->entry_time = APROF_(time)();
	#endif

	return tdata;

}

void APROF_(thread_exit)(ThreadId tid){
	
	APROF_(current_TID) = VG_INVALID_THREADID;
	APROF_(current_tdata) = NULL;
	
	#if VERBOSE
	VG_(printf)("Exit thread %d\n", tid);
	#endif
	
	#if DEBUG
	AP_ASSERT(tid - 1 < VG_N_THREADS && tid > 0, "Invalid tid")
	#endif
	ThreadData * tdata = threads[tid - 1];
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Invalid tdata")
	#endif
	
	/* Unregister thread info */
	threads[tid -1] = NULL;
	APROF_(current_TID) =  VG_INVALID_THREADID;
	APROF_(current_tdata) = NULL;
	
	#if EVENTCOUNT
	VG_(printf)("[TID=%d] Load: %llu\n", tid, tdata->num_read);
	VG_(printf)("[TID=%d] Store: %llu\n", tid, tdata->num_write);
	VG_(printf)("[TID=%d] Modify: %llu\n", tid, tdata->num_modify);
	VG_(printf)("[TID=%d] Function entry: %llu\n", tid, tdata->num_func_enter);
	VG_(printf)("[TID=%d] Function exit: %llu\n", tid, tdata->num_func_exit);
	VG_(printf)("[TID=%d] Total accesses: %llu\n", tid, 
													tdata->num_modify +
													tdata->num_write +
													tdata->num_read
													);
	#endif
	
	#if SUF2_SEARCH == STATS
	VG_(printf)("[TID=%d] Average stack depth: %llu / %llu = %llu\n", 
					tid, tdata->avg_depth, ops, tdata->avg_depth/ops);
	VG_(printf)("[TID=%d] Average # iterations: %llu / %llu = %llu\n", 
					tid, tdata->avg_iteration, ops, tdata->avg_iteration/ops);
	#endif
	
	#if EMPTY_ANALYSIS 
	HT_destruct(tdata->routine_hash_table);
	VG_(free)(tdata->stack);
	VG_(free)(tdata);
	return;
	#endif
	
	#if 0
	/* Some functions are not transformed in routines: */
	HT_ResetIter(fn_ht);
	Function * fn = HT_Next(fn_ht);
	while(fn != NULL) {
		
		RoutineInfo * rtn = HT_lookup(tdata->routine_hash_table, (UWord) fn);
		if (rtn == NULL) 
		VG_(printf)("Function %s is not a routine for this thread\n", fn->name);
		
		fn = HT_Next(fn_ht);
	}
	#endif
	
	APROF_(generate_report)(tdata, tid);
	
	/* destroy all thread data data */
	
	LK_destroy(tdata->accesses);
	
	#if CCT
	// deallocate CCT
	APROF_(free_CCT)(tdata->root);
	#endif
	
	HT_destruct(tdata->routine_hash_table);
	
	VG_(free)(tdata->stack);
	VG_(free)(tdata);

}

void APROF_(switch_thread)(ThreadId tid, ULong blocks_dispatched) {
	
	/* 
	 * Note: Callgrind says that if you have not done at least
	 * 5000 blocks you can ignore this call.
		-----
		static ULong last_blocks_done = 0;
		// throttle calls to CLG_(run_thread) by number of BBs executed
		if (blocks_done - blocks_dispatched < 5000) return;
		------
	 * Why? Investigate! 
	 */
	 
	if (tid == APROF_(current_TID)) return;
	
	#if TRACE_FUNCTION
	/* save last exit of the previous thread */
	if (APROF_(current_tdata) != NULL)
		APROF_(current_tdata)->last_exit = APROF_(last_exit);
	#endif
	
	APROF_(current_TID) = tid;
	
	if (tid == VG_INVALID_THREADID) {
		APROF_(current_tdata) = NULL;
		return;
	}
	
	if (threads[tid-1] == NULL) 
		APROF_(current_tdata) = APROF_(thread_start)(tid);
	else 
		APROF_(current_tdata) = threads[tid -1];
	
	#if DEBUG
	AP_ASSERT(APROF_(current_tdata) != NULL, "Invalid tdata");
	#endif
	
	#if TRACE_FUNCTION
	/* restore exit value of the current thread */
	APROF_(last_exit) = APROF_(current_tdata)->last_exit;
	#endif
}


