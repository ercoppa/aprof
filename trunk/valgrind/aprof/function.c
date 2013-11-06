/*
 * Entry and exit function handler
 * 
 * Last changed: $Date: 2013-08-30 11:29:14 +0200 (ven, 30 ago 2013) $
 * Revision:     $Rev: 884 $
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2013, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi,
                            Romolo Marotta

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
    
    #else // !CCT
    
    #if INPUT_METRIC == RMS || DEBUG_DRMS
    HT_destruct(ri->rms_map);
    #endif // INPUT_METRIC == RMS || DEBUG_DRMS
    
    #if INPUT_METRIC == RVMS
    
    HT_destruct(ri->rvms_map);
    
    #if DISTINCT_RMS
    HT_destruct(ri->distinct_rms);
    #endif // DISTINCT_RMS
    
    #endif // INPUT_METRIC == RVMS
    
    #endif // !CCT
    
    VG_(free)(ri);
    #if DEBUG_ALLOCATION
    APROF_(remove_alloc)(RT_S);
    #endif // DEBUG_ALLOCATION

}

RoutineInfo * APROF_(new_routine_info)(ThreadData * tdata, 
                                Function * fn, UWord target) {
    
    #if DEBUG
    AP_ASSERT(tdata != NULL, "Thread data is not valid");
    AP_ASSERT(fn != NULL, "Invalid function info");
    AP_ASSERT(target > 0, "Invalid target");
    #endif
    
    RoutineInfo * rtn_info = VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
    
    #if DEBUG_ALLOCATION
    APROF_(add_alloc)(RT_S);
    #endif
    
    rtn_info->key = target;
    rtn_info->fn = fn;
    
    #if DISCARD_UNKNOWN
    if (!rtn_info->fn->discard_info) {
    #endif
    
    rtn_info->routine_id = tdata->next_routine_id++;

    #if CCT
    /* elements of this ht are freed when we generate the report */
    rtn_info->context_rms_map = HT_construct(NULL);
    #else // CCT == 0
    
    #if INPUT_METRIC == RMS || DEBUG_DRMS
    /* elements of this ht are freed when we generate the report */
    rtn_info->rms_map = HT_construct(NULL);
    #endif 
    
    #if INPUT_METRIC == RVMS
    
    rtn_info->rvms_map = HT_construct(NULL);
    
    #if DISTINCT_RMS
    rtn_info->distinct_rms = HT_construct(NULL);    
    #endif
    
    #endif // INPUT_METRIC == RVMS
    
    #endif // CCT == 0
    
    #if DISCARD_UNKNOWN
    }
    #endif
    
    HT_add_node(tdata->routine_hash_table, rtn_info->key, rtn_info);
    
    return rtn_info;
}

#if TIME == RDTSC
static ULong time = 0;
#endif

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
    
    ULong start = APROF_(time)(tdata);
    
    #if TIME == RDTSC
    if (time > start) {
        VG_(printf)("START=%llu END=%llu sizeof(ULong)=%lu\n", time, start, sizeof(ULong));
        AP_ASSERT(time <= start, "Invalid time");
    }
    time = start;
    #endif

    RoutineInfo * rtn_info = act->rtn_info;
    #if DEBUG
    AP_ASSERT(rtn_info != NULL, "Invalid rtn info");
    #endif

    rtn_info->recursion_pending++;
    act->rtn_info            = rtn_info;
    act->entry_time          = start;
    act->total_children_time = 0;
    
    #if INPUT_METRIC == RMS || DEBUG_DRMS
    
    act->rms                     = 0;
    act->aid_rms                 = tdata->next_aid++;
    if (tdata->next_aid == 0)  // check & fix timestamp overflow
        tdata->next_aid = APROF_(overflow_handler_rms)();
    
    #endif // INPUT_METRIC == RMS || DEBUG_DRMS
    
    #if INPUT_METRIC == RVMS

    act->rvms                = 0;
    act->aid_rvms            = ++APROF_(global_counter);
    if (APROF_(global_counter) == MAX_COUNT_VAL) {  // check & fix timestamp overflow
        tdata->stack_depth--;
        act->aid_rvms = APROF_(global_counter) = APROF_(overflow_handler)();
        tdata->stack_depth++;
    }
    
    #if DISTINCT_RMS
    act->d_rms = 0;
    #endif // DISTINCT_RMS
    
    #if INPUT_STATS
    act->rvms_syscall = 0;
    act->rvms_thread = 0;
    act->rvms_syscall_self = 0;
    act->rvms_thread_self = 0;
    #endif
    
    #endif // INPUT_METRIC == RVMS
    
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
        APROF_(add_alloc)(CCT_S);
        #endif

        // add node to tree
        cnode->nextSibling = parent->firstChild;
        parent->firstChild = cnode;
        #if CCT_GRAPHIC
        cnode->name = act->rtn_info->fn->name;
        #endif
        cnode->routine_id = act->rtn_info->routine_id;
        cnode->context_id = tdata->next_context_id++;
        
        #if CCT_GRAPHIC
        cnode->name = act->rtn_info->fn->name;
        #endif
    }

    // store context node into current activation record
    act->node = cnode;
    #endif // CCT
    
    #if DISCARD_UNKNOWN
    }
    #endif
    
    #if TRACE_FUNCTION && IGNORE_DL_RUNTIME
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

    ULong start = APROF_(time)(tdata);
    
    #if TIME == RDTSC
    if (time > start) {
        VG_(printf)("START=%llu END=%llu sizeof(ULong)=%lu\n", time, start, sizeof(ULong));
        AP_ASSERT(time <= start, "Invalid time");
    }
    time = start;
    #endif
    
    RoutineInfo * rtn_info = act->rtn_info;

    ULong partial_cumulative = start - act->entry_time;
    
    #if DISCARD_UNKNOWN
    if (!rtn_info->fn->discard_info) {
    #endif
    
    // check if routine has ever been called with this RMS
    RMSInfo * info_access = NULL;
    
    #if CCT
    HashTable * map = HT_lookup(rtn_info->context_rms_map, 
                                            act->node->context_id);
    
    if (rms_map == NULL) {
        
        map = HT_construct(NULL);
        #if DEBUG
        AP_ASSERT(map != NULL, "sms_map not allocable");
        #endif
        
        map->key = act->node->context_id;
        HT_add_node(rtn_info->context_rms_map, map->key, map);

    } else {

        info_access = HT_lookup(map, act->rms);
        
    }
    #else // CCT == 0
    
    #if INPUT_METRIC == RMS
    info_access = HT_lookup(rtn_info->rms_map, act->rms);
    #elif INPUT_METRIC == RVMS
    info_access = HT_lookup(rtn_info->rvms_map, act->rvms);
    #endif
    
    #endif // CCT == 0
    
    // make new unique RMS entry
    if (info_access == NULL) {
        
        info_access = (RMSInfo * ) VG_(calloc)("rms_info", 1, sizeof(RMSInfo));
        #if DEBUG
        AP_ASSERT(info_access != NULL, "rms_info not allocable in function exit");
        #endif
        
        #if DEBUG_ALLOCATION
        APROF_(add_alloc)(RMS_S);
        #endif
        
        // init minimum cumulative time for sms entry
        info_access->min_cumulative_time = (ULong)-1;
        info_access->self_time_min = (ULong)-1;
        
        #if INPUT_METRIC == RMS
        info_access->key = act->rms;
        #elif INPUT_METRIC == RVMS
        info_access->key = act->rvms;
        #endif
        
        #if CCT
        
        HT_add_node(map, info_access->key, info_access);
        
        #else
        
        #if INPUT_METRIC == RMS
        HT_add_node(rtn_info->rms_map, info_access->key, info_access);
        #elif INPUT_METRIC == RVMS
        HT_add_node(rtn_info->rvms_map, info_access->key, info_access);
        #endif
        
        #endif

    }

    #if DISTINCT_RMS
    
    //AP_ASSERT(act->d_rms <= act->rvms, "Wrong!");
    
    RMSValue * node = (RMSValue *) HT_lookup(rtn_info->distinct_rms, act->d_rms);
    if (node == NULL) {
    
        node = (RMSValue *) VG_(calloc)("distinct rms node", sizeof(RMSValue), 1);
        node->key = act->d_rms;
        node->calls = 1;
        HT_add_node(rtn_info->distinct_rms, node->key, node);
        
        #if DEBUG_ALLOCATION
        APROF_(add_alloc)(HTN_S);
        #endif
    
    } else
        node->calls++;
    #endif

    // bookkeeping
    
    info_access->calls_number++;
    
    /* cumulative cost */
    info_access->cumulative_time_sum += partial_cumulative;
    info_access->cumulative_sum_sqr += (partial_cumulative*partial_cumulative);
    
    if (info_access->max_cumulative_time < partial_cumulative) 
        info_access->max_cumulative_time = partial_cumulative;
    
    if (info_access->min_cumulative_time > partial_cumulative) 
        info_access->min_cumulative_time = partial_cumulative;
    
    /* real cumulative cost */
    if (rtn_info->recursion_pending < 2) 
        info_access->cumul_real_time_sum += partial_cumulative;

    /* self cost */
    ULong partial_self = partial_cumulative - act->total_children_time;
    info_access->self_time_sum += partial_self;
    info_access->self_sum_sqr += (partial_self*partial_self);
    
    if (info_access->self_time_max < partial_self) 
        info_access->self_time_max = partial_self;
    
    if (info_access->self_time_min > partial_self) 
        info_access->self_time_min = partial_self;
    
    #if INPUT_METRIC == RVMS && DISTINCT_RMS
    info_access->rms_input_sum += act->d_rms;
    info_access->rms_input_sum_sqr += (act->d_rms * act->d_rms);
    #endif
    
    #if INPUT_STATS
    AP_ASSERT(act->rvms_syscall <= act->rvms, "Wrong");
    AP_ASSERT(act->rvms_thread <= act->rvms, "Wrong");
    info_access->rvms_syscall_sum += act->rvms_syscall;
    info_access->rvms_thread_sum  += act->rvms_thread;
    info_access->rvms_syscall_self += act->rvms_syscall_self;
    info_access->rvms_thread_self  += act->rvms_thread_self;
    #endif
    
    #if DEBUG
    AP_ASSERT(info_access->max_cumulative_time >= 
        info_access->min_cumulative_time, "Min max mismatch");
    if (info_access->calls_number == 1)
        AP_ASSERT(info_access->min_cumulative_time == 
        info_access->max_cumulative_time, "Min max mismatch");
    #endif
    
    #if DISCARD_UNKNOWN
    }
    #endif
    
    rtn_info->recursion_pending--;

    // merge accesses of current activation with those of the parent activation
    if (tdata->stack_depth > 1) {

        Activation * parent_activation = APROF_(get_activation_noresize)(
                                        tdata, tdata->stack_depth - 1);
        #if DEBUG
        AP_ASSERT(parent_activation != NULL, "Invalid parent activation");
        #endif

        #if TRACE_FUNCTION && IGNORE_DL_RUNTIME
        if (!tdata->skip) {
        #endif

            #if DEBUG_DRMS || INPUT_METRIC == RMS
            parent_activation->rms                 += act->rms;
            #endif
            
            #if INPUT_METRIC == RVMS
            parent_activation->rvms                += act->rvms;
            
            #if DISTINCT_RMS
            parent_activation->d_rms               += act->d_rms;
            #endif
            
            #if INPUT_STATS
            parent_activation->rvms_syscall        += act->rvms_syscall;
            parent_activation->rvms_thread         += act->rvms_thread;
            #endif
            
            #endif
        
            parent_activation->total_children_time += partial_cumulative;
        
        #if TRACE_FUNCTION && IGNORE_DL_RUNTIME
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
    

    #if TRACE_FUNCTION && IGNORE_DL_RUNTIME 
    if (act->skip) tdata->skip = False;
    #endif

}
