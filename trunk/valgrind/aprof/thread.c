/*
 * Thread event handlers (start, exit, running)
 * 
 * Last changed: $Date: 2011-07-08 00:14:45 +0200 (Fri, 08 Jul 2011) $
 * Revision:     $Rev: 70 $
 */

#include "aprof.h"
 
/* FixMe:
 * - context_sms_map chi la distrugge? Dove e quando?
 */

static HashTable * thread_pool = NULL;
#if EVENTCOUNT
UWord thread_n = 0;
#endif

void thread_pool_init(void) {
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	thread_pool = HT_construct(NULL);
	if (thread_pool == NULL) failure("thread pool not allocatable");
}

ThreadData * thread_init(ThreadId tid){

	#if EMPTY_ANALYSIS
	return NULL;
	#endif

	#if VERBOSE && !TRACER
	//VG_(printf)("Init thread data %d\n", tid);
	#endif
	
	ThreadData * tdata = VG_(malloc)("thread_data", sizeof(ThreadData));
	if (tdata == NULL) failure("thread data not allocable");
	
	#if TRACER
	Char filename[150];
	Char * prog_name = (Char *) VG_(args_the_exename);
	VG_(sprintf)(filename, "%s_%u.suf", prog_name, tid);
	tdata->log = ap_fopen(filename);
	tdata->stack_depth = 0;
	HT_add_node(thread_pool, tid, tdata);
	return tdata;
	#endif

	tdata->accesses = UF_create();
	tdata->routine_hash_table = HT_construct(destroy_routine_info);
	tdata->stack_depth = 0;
	tdata->max_stack_size = STACK_SIZE;
	tdata->stack = VG_(malloc)("stack", STACK_SIZE * sizeof(Activation));
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
	
	/* Insert thread data in the thread pool */
	HT_add_node(thread_pool, tid, tdata);

	return tdata;

}

void thread_start(ThreadId tid) {
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	#if VERBOSE && !TRACER
	VG_(printf)("Start thread %u\n", tid);
	#endif
	
	#if EVENTCOUNT
	thread_n++;
	#endif
	
	thread_init(tid);
}

void thread_exit (ThreadId tid){

	#if EMPTY_ANALYSIS
	return;
	#endif

	#if VERBOSE && !TRACER
	VG_(printf)("Exit thread %d\n", tid);
	#endif

	ThreadData * tdata = HT_lookup(thread_pool, tid, NULL);
	
	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in thread exit");
	#endif
	
	#if TRACER
	ap_fclose(tdata->log);
	HT_remove(thread_pool, tid);
	VG_(free)(tdata);
	return;
	#endif
	
	generate_report(tdata);
	
	/* destroy all thread data data */
	UF_destroy(tdata->accesses);
	HT_destruct(tdata->routine_hash_table);
	#if CCT
	// deallocate CCT
	freeTree(tdata->root);
	#endif
	
	VG_(free)(tdata->stack);
	HT_remove(thread_pool, tid);
	VG_(free)(tdata);

}

ThreadId thread_running (void) {
	return VG_(get_running_tid)();
}

ThreadData * get_thread_data(ThreadId tid) {

	if (tid == 0) /* tid == 0, we ask to Valgrind */
		tid = VG_(get_running_tid)();
	
	if (tid == 0) failure("Thread id is zero");
	
	ThreadData * tdata = HT_lookup(thread_pool, tid, NULL);
	if (tdata == NULL)
		return thread_init(tid);
	
	return tdata;

}

