/*
 * Entry and exit function handler, simulated thread stack handler
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"
 
#if EVENTCOUNT
UWord fn_in_n  = 0;
UWord fn_out_n = 0;
#endif

#if SUF2_SEARCH == STATS

#endif

Activation * get_activation(ThreadData * tdata, unsigned int depth) {

	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in get_activation");
	if (depth == 0) failure("Inavalid depth in get_activation");
	#endif

	/* Expand stack if necessary */
	if (depth - 1 >= tdata->max_stack_size) {
		
		tdata->max_stack_size = tdata->max_stack_size * 2;
		
		#if VERBOSE
		VG_(printf)("Relocate stack\n");
		#endif
		
		tdata->stack = VG_(realloc)("stack", tdata->stack, tdata->max_stack_size * sizeof(Activation));
		if (tdata->stack == NULL) failure("stack not reallocable");
	
	}
	
	return tdata->stack + depth - 1;

}

#if SUF == 2

/*
static void dump_stack(ThreadData * tdata, UWord aid) {
	
	VG_(printf)("\n");
	VG_(printf)("Searching aid: %lu", aid);
	VG_(printf)("\nStack depth: %lu ", tdata->stack_depth);
	VG_(printf)("\n");
	Activation * a = get_activation(tdata, tdata->stack_depth);
	while (a >= tdata->stack) {
		VG_(printf)("%lu ", a->aid);
		a--;
	}
	VG_(printf)("\n");
	
}
*/

#if SUF2_SEARCH == STATS
UWord64 avg_depth = 0;
UWord64 avg_iteration = 0;
UWord64 ops = 0;
#endif

Activation * get_activation_by_aid(ThreadData * tdata, UWord aid) {
	
	/* Linear search... maybe better binary search */
	#if SUF2_SEARCH == LINEAR
	
	Activation * a = &tdata->stack[tdata->stack_depth - 2];
	while (a->aid > aid) {
		
		a--;
		if (a < tdata->stack) failure("Impossible");
		
	}
	return a;
	
	#elif SUF2_SEARCH == BINARY
	
	Word min = 0;
	Word max = tdata->stack_depth - 2;
	
	do {
		
		Word index = (min + max) / 2;
		
		if (tdata->stack[index].aid == aid) {
			//if (a != &tdata->stack[index]) failure("Search wrong");
			return &tdata->stack[index];
		}
		
		else if (tdata->stack[index].aid > aid) 
			max = index - 1; 
			
		else {
			
			if (tdata->stack[index + 1].aid > aid) {
				//if (a != &tdata->stack[index]) failure("Search wrong");
				return &tdata->stack[index];
			}
			min = index + 1;
			
		}
		
	} while(min <= max);
	
	failure("Binary search fail");
	return NULL;

	#elif SUF2_SEARCH == STATS
	
	ops++;
	avg_depth += tdata->stack_depth;
	avg_iteration++;
	
	Activation * a = get_activation(tdata, tdata->stack_depth - 1);
	while (a->aid > aid) {
		a--;
		avg_iteration++;
		if (a < tdata->stack) failure("Impossible");
	}
	return a;
	
	#endif

}
#endif

void destroy_routine_info(void * data) {
	
	RoutineInfo * ri = (RoutineInfo *) data;
	#if CCT
	HT_destruct(ri->context_sms_map);
	#else
	HT_destruct(ri->sms_map);
	#endif
	VG_(free)(ri);

}

#if !TRACE_FUNCTION
Bool trace_function(ThreadId tid, UWord * arg, UWord * ret) {
	
	/* Is this client request for aprof? */
	if (!VG_IS_TOOL_USERREQ('V','A',arg[0])) return False;
	
	#if EMPTY_ANALYSIS
	return True;
	#endif
	
	ThreadData * tdata = get_thread_data(tid);
	UWord target = arg[1]; 
	
	if (arg[2] == 1) 
		function_enter(tdata, target);
	else if (arg[2] == 2)
		function_exit(tdata, target);
	else
		failure("Invalid client req\n");
	
	return True;

}
#endif

#if TRACE_FUNCTION
void function_enter(ThreadData * tdata, UWord target, 
						char * rtn_name, char * image_name) {
#else
void function_enter(ThreadData * tdata, UWord target) {
#endif

	/*
	tdata->stack_depth++;
	
	int i = 0;
	for(i = 0; i < tdata->stack_depth -1; i++)
		VG_(printf)("| ");
	
	VG_(printf)("> %s\n", rtn_name);
	
	return;
	*/
	
	#if EVENTCOUNT >= 2
	fn_in_n++;
	return;
	#endif
	
	UWord64 start = ap_time();
	
	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in trace_function");
	#endif
	
	#if VERBOSE
	VG_(printf)("Start function: %s\n", debug_name); 
	#endif

	tdata->stack_depth++;

	/* check if routine has already been encountered */
	RoutineInfo * rtn_info = HT_lookup(tdata->routine_hash_table, target, NULL); 
	
	/* new routine, make routine record */
	if (rtn_info == NULL) {
		
		#if !TRACE_FUNCTION
		/* Get routine name */
		char * rtn_name = VG_(calloc)("rtn_name", 256, 1);
		if (rtn_name == NULL) failure("rtn_name not allocable");
		if (!VG_(get_fnname)(target, rtn_name, 256)) 
			VG_(sprintf)(rtn_name, "???");
		DebugInfo * di = VG_(find_DebugInfo)(target);
		if (di == NULL) failure("Invalid DebugInfo");
		char * image_name = (Char *) VG_(DebugInfo_get_soname)(di);
		#endif
		
		rtn_info = (RoutineInfo * )VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
		if (rtn_info == NULL) failure("rtn_info not allocable");
		rtn_info->routine_id = tdata->next_routine_id++;
		rtn_info->name       = rtn_name;
		rtn_info->image_name = image_name;
		//VG_(printf)("New routine info: %s %lu\n", rtn_name, rtn_info->routine_id);
		
		#if CCT
		rtn_info->context_sms_map = HT_construct(HT_destruct);
		if (rtn_info->context_sms_map == NULL) failure("context_sms_map not allocable");
		#else
		rtn_info->sms_map = HT_construct(VG_(free));
		if (rtn_info->sms_map == NULL) failure("sms_map not allocable");
		#endif
		
		/* add routine to routine to ht */
		HT_add_node(tdata->routine_hash_table, target, rtn_info);
		
	}
	
	// bookkeeping
	rtn_info->calls++;
	rtn_info->recursion_pending++;
	if (rtn_info->recursion_pending > 1) rtn_info->recursive = 1;
	
	Activation * act = get_activation(tdata, tdata->stack_depth);
	#if DEBUG
	if (act == NULL) failure("Invalid activation in function entry");
	#endif
	act->rtn_info            = rtn_info;
	act->entry_time          = start;
	act->sms                 = 0;
	act->total_children_time = 0;
	#if SUF == 2
	act->aid                 = tdata->next_aid++;
	act->old_aid             = tdata->curr_aid;
	tdata->curr_aid                 = act->aid;
	#endif
	
	#if CCT
	
	CCTNode * parent = parent_CCT(tdata);
	#if DEBUG
	if (parent == NULL) failure("Invalid parent CCT in function entry");
	#endif 
	CCTNode * cnode = parent->firstChild;
	
	// did we already encounter this context?
	while (cnode != NULL) {
		
		if (cnode->target == target) break;
		cnode = cnode->nextSibling;
	
	}

	if (cnode == NULL) {
		
		// create new context node
		cnode = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
		if (cnode == NULL) 
			failure("Can't allocate CTT node\n");

		// add node to tree
		cnode->nextSibling = parent->firstChild;
		parent->firstChild = cnode;
		cnode->target = target;
		cnode->routine_id = act->rtn_info->routine_id;
		cnode->context_id = tdata->next_context_id++;
		
	}

	// store context node into current activation record
	act->node = cnode;
	#endif
	
	act->overhead = 0;
	
}

void function_exit(ThreadData * tdata, UWord target) {
	
	/*
	tdata->stack_depth--;
	return;
	*/
	
	#if EVENTCOUNT >= 2
	fn_out_n++;
	return;
	#endif
	
	UWord64 start = ap_time();
	
	Activation * act = get_activation(tdata, tdata->stack_depth);
	#if DEBUG
	if (act == NULL) failure("Invalid activation in function exit");
	#endif
	RoutineInfo * rtn_info = act->rtn_info;

	UWord64 partial_cumulative =  start - act->entry_time - act->overhead;
	if (rtn_info->recursion_pending < 2) rtn_info->total_cumulative_time += partial_cumulative;

	UWord partial_self = partial_cumulative - act->total_children_time;
	rtn_info->total_self_time += partial_self;
	
	// check if routine has ever been called with this seen memory size (SMS)
	SMSInfo * info_access = NULL;
	#if CCT
	HashTable * sms_map  = HT_lookup(rtn_info->context_sms_map, 
									act->node->context_id, NULL);
	
	if (sms_map == NULL) {
		
		//VG_(printf)("New sms map\n");
		sms_map = HT_construct(VG_(free));
		if (sms_map == NULL) failure("sms_map not allocable");
		HT_add_node(rtn_info->context_sms_map, act->node->context_id, sms_map);

	} else {
		
		info_access = HT_lookup(sms_map, act->sms, NULL);
	
	}
	#else
	
	info_access = HT_lookup(rtn_info->sms_map, act->sms, NULL);
	
	#endif
	
	// make new unique SMS entry
	if (info_access == NULL) {
		
		//VG_(printf)("New sms info\n");
		info_access = (SMSInfo * ) VG_(calloc)("sms_info", 1, sizeof(SMSInfo));
		if (info_access == NULL) failure("sms_info not allocable in function exit");
		info_access->sms = act->sms;
		#if CCT
		HT_add_node(sms_map, info_access->sms, info_access);
		#else
		HT_add_node(rtn_info->sms_map, info_access->sms, info_access);
		#endif

	}

	// bookkeeping
	info_access->partial_cumulative_time += partial_cumulative;
	info_access->partial_calls_number++;
	if (info_access->max_cumulative_time < partial_cumulative) info_access->max_cumulative_time = partial_cumulative;
	if (info_access->min_cumulative_time > partial_cumulative) info_access->min_cumulative_time = partial_cumulative;
	rtn_info->recursion_pending--;
	

	// merge accesses of current activation with those of the parent activation
	if (tdata->stack_depth > 1) {

		Activation * parent_activation = get_activation(tdata, tdata->stack_depth - 1);
		#ifdef DEBUG
		if (parent_activation == NULL) failure("Invalid parent in function exit");
		#endif
		
		#if SUF == 1
		UF_merge(tdata->accesses, tdata->stack_depth);
		#elif SUF == 2
		tdata->curr_aid = act->old_aid;
		#endif

		parent_activation->sms                 += act->sms;
		parent_activation->total_children_time += partial_cumulative;
		
	}
	
	if (tdata->stack_depth > 0)
		tdata->stack_depth--;

}
