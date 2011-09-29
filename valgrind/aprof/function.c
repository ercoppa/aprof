/*
 * Entry and exit function handler
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"
 
#if EVENTCOUNT
UWord fn_in_n  = 0;
UWord fn_out_n = 0;
#endif

pool_t * rtn_pool = NULL;
void * rtn_free_list = NULL;
pool_t * sms_pool = NULL;
void * sms_free_list = NULL;

void destroy_routine_info(void * data) {
	
	RoutineInfo * ri = (RoutineInfo *) data;
	#if CCT
	HT_destruct(ri->context_sms_map);
	#else
	HT_destruct(ri->sms_map);
	#endif
	pool_free(ri, rtn_free_list);
	//VG_(free)(ri);

}

RoutineInfo * new_routine_info(ThreadData * tdata, Function * fn, UWord target) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(fn != NULL, "Invalid function info");
	AP_ASSERT(target > 0, "Invalid target");
	#endif
	
	RoutineInfo * rtn_info = NULL;
	pool_alloc(rtn_pool, rtn_free_list, rtn_info, RoutineInfo);
	rtn_info->next = NULL;
	
	//RoutineInfo * rtn_info = VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
	#if DEBUG
	AP_ASSERT(rtn_info != NULL, "rtn info not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	add_alloc(RTS);
	#endif
	
	rtn_info->key = target;
	rtn_info->routine_id = tdata->next_routine_id++;
	rtn_info->fn = fn;
	
	rtn_info->total_self_time = 0;
	rtn_info->total_cumulative_time = 0;
	rtn_info->calls = 0;
	rtn_info->recursive = 0;
	rtn_info->recursion_pending = 0;
	
	#if CCT
	rtn_info->context_sms_map = HT_construct(HT_destruct);
	#if DEBUG
	AP_ASSERT(rtn_info->context_sms_map != NULL, "context_sms_map not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	add_alloc(HT);
	#endif
	
	#else
	
	rtn_info->sms_map = HT_construct(NULL);
	#if DEBUG
	AP_ASSERT(rtn_info->sms_map != NULL, "sms_map not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	add_alloc(HT);
	#endif
	
	#endif
	
	HT_add_node(tdata->routine_hash_table, rtn_info->key, rtn_info);
	
	#if DEBUG_ALLOCATION
	add_alloc(HTN);
	#endif
	
	return rtn_info;
}

void function_enter(ThreadData * tdata, Activation * act) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	#endif
	
	#if VERBOSE == 5
	/*
	int i = 0;
	for(i = 0; i < tdata->stack_depth - 1; i++)
		VG_(printf)("| ");
	*/
	VG_(printf)("[%lu] %s\n", tdata->stack_depth, act->rtn_info->fn->name);
	return;
	#endif
	
	UWord64 start = ap_time();

	RoutineInfo * rtn_info = act->rtn_info;
	#if DEBUG
	AP_ASSERT(rtn_info != NULL, "Invalid rtn info");
	#endif
	
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
	
	CCTNode * parent = parent_CCT(tdata);
	#if DEBUG
	AP_ASSERT(parent != NULL, "Invalid parent CCT");
	#endif
	
	CCTNode * cnode = parent->firstChild;
	
	// did we already encounter this context?
	while (cnode != NULL) {
		
		if (cnode->routine_id == act->rtn_info->routine_id) break;
		cnode = cnode->nextSibling;
	
	}

	if (cnode == NULL) {
		
		// create new context node
		cnode = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
		AP_ASSERT(cnode != NULL, "Can't allocate CTT node");

		#if DEBUG_ALLOCATION
		add_alloc(CCTS);
		#endif

		// add node to tree
		cnode->nextSibling = parent->firstChild;
		parent->firstChild = cnode;
		#if CCT_GRAPHIC
		cnode->name = act->rtn_info->fn->name;
		#endif
		cnode->routine_id = act->rtn_info->routine_id;
		cnode->context_id = tdata->next_context_id++;
		
	}

	// store context node into current activation record
	act->node = cnode;
	#endif
	
	#if TRACE_FUNCTION
	if (act->skip) tdata->skip = True;
	#endif
	
}

void function_exit(ThreadData * tdata, Activation * act) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	#endif
	
	#if VERBOSE == 5
	return;
	#endif
	
	UWord64 start = ap_time();
	RoutineInfo * rtn_info = act->rtn_info;

	UWord64 partial_cumulative =  start - act->entry_time;
	if (rtn_info->recursion_pending < 2) 
		rtn_info->total_cumulative_time += partial_cumulative;

	UWord partial_self = partial_cumulative - act->total_children_time;
	rtn_info->total_self_time += partial_self;
	
	// check if routine has ever been called with this seen memory size (SMS)
	SMSInfo * info_access = NULL;
	#if CCT
	HashTable * sms_map  = HT_lookup(rtn_info->context_sms_map, 
											act->node->context_id);
	
	if (sms_map == NULL) {
		
		//VG_(printf)("New sms map\n");
		sms_map = HT_construct(NULL);
		#if DEBUG
		AP_ASSERT(sms_map != NULL, "sms_map not allocable");
		#endif
		
		#if DEBUG_ALLOCATION
		add_alloc(HT);
		#endif
		
		sms_map->key = act->node->context_id;
		HT_add_node(rtn_info->context_sms_map, sms_map->key, sms_map);

		#if DEBUG_ALLOCATION
		add_alloc(HTN);
		#endif

	} else {
		
		info_access = HT_lookup(sms_map, act->sms);
	
	}
	#else
	
	info_access = HT_lookup(rtn_info->sms_map, act->sms);
	
	#endif
	
	// make new unique SMS entry
	if (info_access == NULL) {
		
		//VG_(printf)("New sms info\n");
		pool_alloc(sms_pool, sms_free_list, info_access, SMSInfo);
		info_access->next = NULL;
		info_access->max_cumulative_time = 0;
		info_access->min_cumulative_time = 0;
		info_access->partial_cumulative_time = 0;
		info_access->partial_calls_number = 0;
		
		//info_access = (SMSInfo * ) VG_(calloc)("sms_info", 1, sizeof(SMSInfo));
		#if DEBUG
		AP_ASSERT(info_access != NULL, "sms_info not allocable in function exit");
		#endif
		
		#if DEBUG_ALLOCATION
		add_alloc(SMS);
		#endif
		
		info_access->key = act->sms;
		#if CCT
		HT_add_node(sms_map, info_access->key, info_access);
		
		#if DEBUG_ALLOCATION
		add_alloc(HTN);
		#endif
		
		#else
		HT_add_node(rtn_info->sms_map, info_access->key, info_access);
		
		#if DEBUG_ALLOCATION
		add_alloc(HTN);
		#endif
		
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
		#if DEBUG
		AP_ASSERT(parent_activation != NULL, "Invalid parent activation");
		#endif

		#if SUF == 1
		UF_merge(tdata->accesses, tdata->stack_depth);
		#elif SUF == 2
		tdata->curr_aid = act->old_aid;
		#endif

		#if TRACE_FUNCTION
		if (!tdata->skip) {
		#endif
			parent_activation->sms                 += act->sms;
			parent_activation->total_children_time += partial_cumulative;
		#if TRACE_FUNCTION
		} else {
			tdata->bb_c -= partial_cumulative;
		}
		#endif
	}
	
	#if TRACE_FUNCTION
	if (act->skip) tdata->skip = False;
	#endif

}
