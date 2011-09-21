/*
 * Thread event handlers (start, exit, running)
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/* All threads */
static ThreadData * threads[VG_N_THREADS];
ThreadId current_TID = 0;
ThreadData * current_tdata = NULL; 

#if EVENTCOUNT
UWord thread_n = 0;
#endif

static ThreadData * thread_start(ThreadId tid){

	#if EMPTY_ANALYSIS
	return NULL;
	#endif

	#if VERBOSE && !TRACER
	//VG_(printf)("Init thread data %d\n", tid);
	#endif
	
	ThreadData * tdata = VG_(calloc)("thread_data", sizeof(ThreadData), 1);
	if (tdata == NULL) failure("thread data not allocable");
	
	threads[tid-1] = tdata;

	#if SUF == 1
	tdata->accesses = UF_create();
	#elif SUF == 2
	tdata->accesses = SUF_create();
	#endif
	
	tdata->routine_hash_table = HT_construct(destroy_routine_info);
	tdata->stack_depth = 0;
	tdata->max_stack_size = STACK_SIZE;
	tdata->stack = VG_(calloc)("stack", STACK_SIZE * sizeof(Activation), 1);
	tdata->next_routine_id = 0;
	#if CCT
	// allocate dummy CCT root
	tdata->root = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
	if (tdata->root == NULL)
		failure("Can't allocate CCT root node");
	tdata->root->context_id = 0;
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
	init_stack(tdata);  
	tdata->inside_main = False;
	#endif

	return tdata;

}

void thread_exit (ThreadId tid){

	#if EMPTY_ANALYSIS || EVENTCOUNT
	return;
	#endif

	#if VERBOSE && !TRACER
	VG_(printf)("Exit thread %d\n", tid);
	#endif

	ThreadData * tdata = NULL;
	tdata = threads[tid - 1];
	
	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in thread exit");
	#endif
	
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
	threads[tid -1] = NULL;
	
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
	if (threads[tid-1] == NULL) thread_start(tid);
	
	if (current_tdata != NULL)
		current_tdata->last_exit = last_exit;
	
	current_TID = tid;
	current_tdata = threads[tid -1];
	last_exit = current_tdata->last_exit;
	
}


