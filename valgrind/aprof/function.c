/*
 * Entry and exit function handler
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

void APROF_(destroy_routine_info)(RoutineInfo * ri) {
	
	#if CCT
	HT_destruct(ri->context_rms_map);
	#else
	HT_destruct(ri->rms_map);
	#endif
	
	VG_(free)(ri);

}

RoutineInfo * APROF_(new_routine_info)(ThreadData * tdata, 
								Function * fn, UWord target) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(fn != NULL, "Invalid function info");
	AP_ASSERT(target > 0, "Invalid target");
	#endif
	
	RoutineInfo * rtn_info = VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
	#if DEBUG
	AP_ASSERT(rtn_info != NULL, "rtn info not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(RTS);
	#endif
	
	rtn_info->key = target;
	rtn_info->fn = fn;
	
	/*
	rtn_info->total_self_time = 0;
	rtn_info->total_cumulative_time = 0;
	rtn_info->calls = 0;
	rtn_info->recursive = 0;
	rtn_info->recursion_pending = 0;
	*/
	
	#if DISCARD_UNKNOWN
	if (!rtn_info->fn->discard_info) {
	#endif
	
	rtn_info->routine_id = tdata->next_routine_id++;

	#if CCT
	
	/* elements of this ht are freed when we generate the report */
	rtn_info->context_rms_map = HT_construct(NULL);
	
	#if DEBUG
	AP_ASSERT(rtn_info->context_rms_map != NULL, "context_sms_map not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	 APROF_(add_alloc)(HT);
	#endif
	
	#else
	
	/* elements of this ht are freed when we generate the report */
	rtn_info->rms_map = HT_construct(NULL);
	#if DEBUG
	AP_ASSERT(rtn_info->rms_map != NULL, "sms_map not allocable");
	#endif
	
	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(HT);
	#endif
	
	#endif
	
	#if DISCARD_UNKNOWN
	}
	#endif
	
	HT_add_node(tdata->routine_hash_table, rtn_info->key, rtn_info);
	
	#if DEBUG_ALLOCATION
	APROF_(add_alloc)(HTN);
	#endif
	
	return rtn_info;
}

void APROF_(function_enter)(ThreadData * tdata, Activation * act) {

	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	#endif
	
	#if VERBOSE
	VG_(printf)("Enter: %s\n", act->rtn_info->fn->name);
	/*
	int i = 0;
	for(i = 0; i < tdata->stack_depth - 1; i++)
		VG_(printf)("| ");
	*/
	//VG_(printf)("[%lu] %s\n", tdata->stack_depth, act->rtn_info->fn->name);
	#endif
	
	#if EVENTCOUNT >= 2
	tdata->num_func_enter++;
	#endif
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	ULong start = APROF_(time)();

	RoutineInfo * rtn_info = act->rtn_info;
	#if DEBUG
	AP_ASSERT(rtn_info != NULL, "Invalid rtn info");
	#endif
	
	rtn_info->calls++;
	rtn_info->recursion_pending++;
	if (rtn_info->recursion_pending > 1) rtn_info->recursive = 1;
	act->rtn_info            = rtn_info;
	act->entry_time          = start;
	act->rms                 = 0;
	act->total_children_time = 0;
	act->aid                 = tdata->next_aid++;
	
	/* check & fix timestamp overflow */
	if (act->aid == 0) {
		
		//SUF_print(tdata->accesses);
		
		//VG_(printf)("\nSUF compress\n");
		
		/* Collect all valid aid */
		UInt * arr_aid = VG_(calloc)("arr rid", tdata->stack_depth - 1, sizeof(UInt));
		int j = 0;
		for (j = 0; j < tdata->stack_depth - 1; j++) {
			Activation * act_c = APROF_(get_activation)(tdata, j + 1);
			arr_aid[j] = act_c->aid;
			act_c->aid = j + 1;
			//VG_(printf)("Aid was %u, now is %u\n", arr_aid[j], j+1);
		}
		LK_compress(tdata->accesses, arr_aid, tdata->stack_depth -1);
		VG_(free)(arr_aid);
		
		tdata->next_aid = tdata->stack_depth;
		act->aid = tdata->next_aid++;
		
		//VG_(printf)("Current aid is %u\nNext aid is %u\n", act->aid, tdata->next_aid);
		
	}
	
	#if DISCARD_UNKNOWN
	if (!rtn_info->fn->discard_info) {
	#endif
	
	#if CCT
	
	CCTNode * parent = APROF_(parent_CCT)(tdata);
	#if DEBUG
	AP_ASSERT(parent != NULL, "Invalid parent CCT");
	#endif
	
	CCTNode * cnode = parent->firstChild;
	
	// did we already see this context?
	while (cnode != NULL) {
		
		if (cnode->routine_id == act->rtn_info->routine_id) break;
		cnode = cnode->nextSibling;
	
	}

	if (cnode == NULL) {
		
		// create new context node
		cnode = (CCTNode*) VG_(calloc)("CCT", sizeof(CCTNode), 1);
		#if DEBUG
		AP_ASSERT(cnode != NULL, "Can't allocate CTT node");
		#endif

		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(CCTS);
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
	
	#if DISCARD_UNKNOWN
	}
	#endif
	
	#if TRACE_FUNCTION
	if (act->skip) tdata->skip = True;
	#endif
	
}

void APROF_(function_exit)(ThreadData * tdata, Activation * act) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Thread data is not valid");
	AP_ASSERT(act != NULL, "Invalid activation info");
	#endif
	
	#if VERBOSE
	VG_(printf)("Exit: %s\n", act->rtn_info->fn->name);
	#endif
	
	#if EVENTCOUNT >= 2
	tdata->num_func_exit++;
	#endif
	
	#if EMPTY_ANALYSIS
	return;
	#endif
	
	ULong start = APROF_(time)();
	RoutineInfo * rtn_info = act->rtn_info;

	ULong partial_cumulative = start - act->entry_time;
	
	#if DISCARD_UNKNOWN
	if (!rtn_info->fn->discard_info) {
	#endif
	
	if (rtn_info->recursion_pending < 2) 
		rtn_info->total_cumulative_time += partial_cumulative;

	ULong partial_self = partial_cumulative - act->total_children_time;
	rtn_info->total_self_time += partial_self;
	
	// check if routine has ever been called with this RMS
	RMSInfo * info_access = NULL;
	#if CCT
	HashTable * rms_map  = HT_lookup(rtn_info->context_rms_map, 
											act->node->context_id);
	
	if (rms_map == NULL) {
		
		
		rms_map = HT_construct(NULL);
		#if DEBUG
		AP_ASSERT(rms_map != NULL, "sms_map not allocable");
		#endif
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(HT);
		#endif
		
		rms_map->key = act->node->context_id;
		HT_add_node(rtn_info->context_rms_map, rms_map->key, rms_map);

		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(HTN);
		#endif

	} else {
		
		info_access = HT_lookup(rms_map, act->rms);
	
	}
	#else
	
	info_access = HT_lookup(rtn_info->rms_map, act->rms);
	
	#endif
	
	// make new unique RMS entry
	if (info_access == NULL) {
		
		//VG_(printf)("New sms info\n");
		info_access = (RMSInfo * ) VG_(calloc)("sms_info", 1, sizeof(RMSInfo));
		#if DEBUG
		AP_ASSERT(info_access != NULL, "rms_info not allocable in function exit");
		#endif
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(RMS);
		#endif
		
		// init minimum cumulative time for sms entry
		info_access->min_cumulative_time = (UInt)-1;
		
		info_access->key = act->rms;
		#if CCT
		HT_add_node(rms_map, info_access->key, info_access);
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(HTN);
		#endif
		
		#else
		HT_add_node(rtn_info->rms_map, info_access->key, info_access);
		
		#if DEBUG_ALLOCATION
		APROF_(add_alloc)(HTN);
		#endif
		
		#endif

	}

	// bookkeeping
	info_access->cumulative_time_sum += partial_cumulative;
	info_access->calls_number++;
	
	info_access->cumulative_time_sqr_sum += 
		partial_cumulative * partial_cumulative;
	
	if (info_access->max_cumulative_time < partial_cumulative) 
		info_access->max_cumulative_time = partial_cumulative;
	
	if (info_access->min_cumulative_time > partial_cumulative) 
		info_access->min_cumulative_time = partial_cumulative;
	
	#if DEBUG
	AP_ASSERT(info_access->max_cumulative_time >= info_access->min_cumulative_time, "Min max mismatch");
	if (info_access->calls_number == 1)
		AP_ASSERT(info_access->min_cumulative_time == info_access->max_cumulative_time, "Min max mismatch");
	#endif
	
	#if DISCARD_UNKNOWN
	}
	#endif
	
	rtn_info->recursion_pending--;
	
	// merge accesses of current activation with those of the parent activation
	if (tdata->stack_depth > 1) {

		Activation * parent_activation = APROF_(get_activation)(tdata, tdata->stack_depth - 1);
		#if DEBUG
		AP_ASSERT(parent_activation != NULL, "Invalid parent activation");
		#endif

		#if TRACE_FUNCTION
		if (!tdata->skip) {
		#endif
		
			parent_activation->rms                 += act->rms;
			parent_activation->total_children_time += partial_cumulative;
		
		#if TRACE_FUNCTION
		} else {
	
			#if TIME == BB_COUNT
			if (act->skip)
				tdata->bb_c -= partial_cumulative;
			#else
	
			AP_ASSERT(0, "With RDTSC you can't ignore dl_runtime_resolve");
			#endif
		}
		#endif
	
	}
	
	//VG_(printf)("SMS: %lu\n", act->sms);
	
	#if TRACE_FUNCTION
	if (act->skip) tdata->skip = False;
	#endif

}
