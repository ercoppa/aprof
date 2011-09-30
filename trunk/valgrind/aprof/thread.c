/*
 * Thread event handlers (start, exit, running)
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/* All threads */
static ThreadData * threads[VG_N_THREADS];
ThreadId current_TID = VG_INVALID_THREADID; /* 0 */
ThreadData * current_tdata = NULL; 

#if EVENTCOUNT
UWord thread_n = 0;
#endif

static ThreadData * thread_start(ThreadId tid){

	#if EMPTY_ANALYSIS
	return NULL;
	#endif

	#if VERBOSE && !TRACER
	VG_(printf)("start thread data %d\n", tid);
	#endif
	
	ThreadData * tdata = VG_(calloc)("thread_data", sizeof(ThreadData), 1);
	AP_ASSERT(tdata != NULL, "thread data not allocable");
	
	#if DEBUG_ALLOCATION
	add_alloc(TS);
	#endif
	
	threads[tid-1] = tdata;

	#if SUF == 1
	tdata->accesses = UF_create();
	#elif SUF == 2
	tdata->accesses = SUF_create();
	#endif
	
	tdata->routine_hash_table = HT_construct(destroy_routine_info);
	AP_ASSERT(tdata->routine_hash_table != NULL, "rtn ht not allocable");
	
	#if DEBUG_ALLOCATION
	add_alloc(HT);
	#endif
	
	tdata->stack_depth = 0;
	tdata->max_stack_size = STACK_SIZE;
	tdata->stack = VG_(calloc)("stack", STACK_SIZE * sizeof(Activation), 1);
	AP_ASSERT(tdata->stack != NULL, "stack not allocable");
	
	#if DEBUG_ALLOCATION
	int j = 0;
	for (j = 0; j < STACK_SIZE; j++) add_alloc(ACT);
	#endif
	
	tdata->next_routine_id = 0;
	
	#if CCT
	// allocate dummy CCT root
	tdata->root = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
	AP_ASSERT(tdata->root != NULL, "Can't allocate CCT root node");

	#if DEBUG_ALLOCATION
	add_alloc(CCTS);
	#endif

	tdata->root->context_id = 0;
	
	#if CCT_GRAPHIC
	char * n = VG_(calloc)("nome root", 32, 1);
	n = "ROOT";
	tdata->root->name = n;
	#endif
	
	tdata->next_context_id = 1;
	#endif
	
	#if TIME == INSTR
	tdata->instr = 0;
	#endif
	
	#if SUF == 2
	tdata->next_aid = 1;
	tdata->curr_aid = 0;
	#endif
	
	#if TRACE_FUNCTION
	tdata->last_bb = NULL;
	#endif

	return tdata;

}

void thread_exit (ThreadId tid){
	
	current_TID = 0;
	current_tdata = NULL;
	
	#if EMPTY_ANALYSIS || EVENTCOUNT
	return;
	#endif

	#if VERBOSE && !TRACER
	VG_(printf)("Exit thread %d\n", tid);
	#endif

	ThreadData * tdata = threads[tid - 1];
	AP_ASSERT(tdata != NULL, "Invalid tdata")
	
	/* Unregister thread info */
	threads[tid -1] = NULL;
	current_TID =  VG_INVALID_THREADID;
	current_tdata = NULL;
	
	generate_report(tdata, tid);
	
	/* destroy all thread data data */
	#if SUF == 1
	UF_destroy(tdata->accesses);
	#elif SUF == 2
	SUF_destroy(tdata->accesses);
	#endif
	
	HT_destruct(tdata->routine_hash_table);
	#if CCT
	// deallocate CCT
	freeTree(tdata->root);
	#endif
	
	VG_(free)(tdata->stack);
	VG_(free)(tdata);

}

void switch_thread(ThreadId tid, ULong blocks_dispatched) {
	
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
	 
	if (tid == current_TID) return;
	
	#if TRACE_FUNCTION
	if (current_tdata != NULL)
		current_tdata->last_exit = last_exit;
	#endif
	
	current_TID = tid;
	
	if (tid == VG_INVALID_THREADID) {
		current_tdata = NULL;
		return;
	}
	
	if (threads[tid-1] == NULL) 
		current_tdata = thread_start(tid);
	else 
		current_tdata = threads[tid -1];
	
	#if DEBUG
	AP_ASSERT(current_tdata != NULL, "Invalid tdata");
	#endif
	
	#if TRACE_FUNCTION
	last_exit = current_tdata->last_exit;
	#endif
}


