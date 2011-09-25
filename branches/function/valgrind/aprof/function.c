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

	if (depth - 1 < 0) return NULL;

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
		if (a < tdata->stack) failure("Requested aid not found in stack!");
		
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

RoutineInfo * new_routine_info(ThreadData * tdata, Function * fn, UWord target) {
	
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(fn != NULL, "Invalid function info");
	AP_ASSERT(target != NULL, "Invalid target");
	
	rtn_info = (RoutineInfo * )VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
	AP_ASSERT(rtn_info != NULL, "rtn info not allocable");
	
	rtn_info->routine_id = tdata->next_routine_id++;
	rtn_info->fn = fn;
	
	rtn_info->total_self_time = 0;
	rtn_info->total_cumulative_time = 0;
	rtn_info->calls = 0;
	rtn_info->recursive = 0;
	rtn_info->recursive_pending = 0;
	
	#if CCT
	rtn_info->context_sms_map = HT_construct(HT_destruct);
	AP_ASSERT(rtn_info->context_sms_map != NULL, "context_sms_map not allocable");
	
	#else
	rtn_info->sms_map = HT_construct(VG_(free));
	AP_ASSERT(rtn_info->sms_map != NULL, "sms_map not allocable");
	#endif
	
	HT_add_node(tdata->routine_hash_table, target, rtn_info);
	
}

void function_enter(ThreadData * tdata, Activation * act) {
	
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	
	UWord64 start = ap_time();

	RoutineInfo * rtn_info = act->rtn;
	rtn_info->calls++;
	rtn_info->recursion_pending++;
	if (rtn_info->recursion_pending > 1) rtn_info->recursive = 1;
	act->rtn_info            = rtn_info;
	act->entry_time          = start;
	act->sms                 = 0;
	act->total_children_time = 0;
	
	#if SUF == 2
	act->aid                 = tdata->next_aid++;
	act->old_aid             = tdata->curr_aid;
	tdata->curr_aid          = act->aid;
	#endif
	
	#if CCT
	
	/* FixMe */
	AP_ASSERT(0, "CCT does not manage correctly more than one CCT tree");
	
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
	
}

void function_exit(ThreadData * tdata, Activation * act) {
	
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	
	UWord64 start = ap_time();
	RoutineInfo * rtn = act->rtn;

	UWord64 partial_cumulative =  start - act->entry_time;
	if (rtn_info->recursion_pending < 2) 
		rtn_info->total_cumulative_time += partial_cumulative;

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
		AP_ASSERT(sms_map != NULL, "sms_map not allocable");
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
		AP_ASSERT(info_access != NULL, "sms_info not allocable in function exit");
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
	
	if (info_access->max_cumulative_time < partial_cumulative) 
		info_access->max_cumulative_time = partial_cumulative;
	
	if (info_access->min_cumulative_time > partial_cumulative) 
		info_access->min_cumulative_time = partial_cumulative;
	
	rtn_info->recursion_pending--;
	
	// merge accesses of current activation with those of the parent activation
	if (tdata->stack_depth > 1) {

		Activation * parent_activation = get_activation(tdata, tdata->stack_depth - 1);
		AP_ASSERT(parent_activation != NULL, "Invalid parent activation");

		#if SUF == 1
		UF_merge(tdata->accesses, tdata->stack_depth);
		#elif SUF == 2
		tdata->curr_aid = act->old_aid;
		#endif

		parent_activation->sms                 += act->sms;
		parent_activation->total_children_time += partial_cumulative;
		
	}

}
