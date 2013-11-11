/*
 * Entry and exit function handler
 * 
 * Last changed: $Date: 2013-08-30 11:29:14 +0200 (ven, 30 ago 2013) $
 * Revision:     $Rev: 884 $
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2014, Emilio Coppa (ercoppa@gmail.com),
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
    
    HT_destruct(ri->input_map);
    APROF_(delete)(RT_S, ri);
}

RoutineInfo * APROF_(new_routine_info)(ThreadData * tdata, 
                                Function * fn, UWord target) {
    
    APROF_(debug_assert)(tdata != NULL, "Thread data is not valid");
    APROF_(debug_assert)(fn != NULL, "Invalid function info");
    APROF_(debug_assert)(target > 0, "Invalid target");
    
    RoutineInfo * rtn_info = APROF_(new)(RT_S, sizeof(RoutineInfo));
    
    rtn_info->key = target;
    rtn_info->fn = fn;
    
    if (!rtn_info->fn->discard) {

        rtn_info->routine_id = tdata->next_routine_id++;
        rtn_info->input_map = HT_construct(NULL);
    }
    
    HT_add_node(tdata->rtn_ht, rtn_info->key, rtn_info);
    return rtn_info;
}

void APROF_(function_enter)(ThreadData * tdata, Activation * act) {

    APROF_(debug_assert)(tdata != NULL, "Thread data is not valid");
    APROF_(debug_assert)(act != NULL, "Invalid activation info");
    APROF_(verbose)(1, "Enter: %s\n", act->routine_info->fn->name);
    
    #if EVENTCOUNT
    tdata->num_func_enter++;
    #endif
    
    #if EMPTY_ANALYSIS
    return;
    #endif
    
    act->start = APROF_(time)(tdata);
    
    RoutineInfo * rtn_info = act->routine_info;
    APROF_(debug_assert)(rtn_info != NULL, "Invalid routine info");
    rtn_info->recursion_pending++;
    
    if (rtn_info->fn->skip) {
        tdata->skip++;
        return;
    }
    
    if (APROF_(runtime).input_metric == RMS) {
        
        act->activation_id = tdata->next_activation_id++;
        if ((tdata)->next_activation_id == 0)
            tdata->next_activation_id = APROF_(overflow_handler_rms)();
    
    } else { // DRMS
        
        act->activation_id = ++(APROF_(runtime).global_counter);
        if(APROF_(runtime).global_counter == MAX_COUNT_VAL) {
            
            tdata->stack_depth--;
            APROF_(runtime).global_counter = APROF_(overflow_handler_drms)();
            act->activation_id = APROF_(runtime).global_counter;
            tdata->stack_depth++;
        }
    }
    
    act->sum_children_cost = 0;
    act->input_size = 0;
    
    #if INPUT_STATS
    act->cumul_syscall = 0;
    act->cumul_thread = 0;
    act->self_syscall = 0;
    act->self_thread = 0;
    #endif // INPUT_STATS
    
    if (!rtn_info->fn->discard && APROF_(runtime).collect_CCT) {
    
        CCTNode * parent = APROF_(parent_CCT)(tdata);
        APROF_(debug_assert)(parent != NULL, "Invalid parent CCT");
        
        CCTNode * cnode = parent->first_child;
        // did we already see this context?
        while (cnode != NULL) {
            
            if (cnode->routine_id == rtn_info->routine_id) break;
            cnode = cnode->next_sibling;
        
        }

        if (cnode == NULL) {
            
            // create new context node
            cnode = APROF_(new)(CCT_S, sizeof(CCTNode));
            cnode->routine_id = rtn_info->routine_id;
            cnode->context_id = tdata->next_context_id++;

            // add node to tree
            cnode->next_sibling = parent->first_child;
            parent->first_child = cnode;
        }

        // store context node into current activation
        act->node = cnode;
    }
}

void APROF_(function_exit)(ThreadData * tdata, Activation * act) {
    
    APROF_(debug_assert)(tdata != NULL, "Thread data is not valid");
    APROF_(debug_assert)(act != NULL, "Invalid activation info");
    APROF_(verbose)(1, "Exit: %s\n", act->routine_info->fn->name);
    
    #if EVENTCOUNT
    tdata->num_func_exit++;
    #endif
    
    #if EMPTY_ANALYSIS
    return;
    #endif

    ULong cumul_cost = APROF_(time)(tdata) - act->start;
    ULong self_cost = cumul_cost - act->sum_children_cost;
    
    RoutineInfo * rtn_info = act->routine_info;
    APROF_(debug_assert)(rtn_info != NULL, "Invalid routine info");
    
    if (tdata->skip > 0) {
        
        if (tdata->skip == 1)
            tdata->skip_cost += cumul_cost;
        
        tdata->skip--;
        return;
    }

    if (!rtn_info->fn->discard) {
    
        UWord key;
        if (APROF_(runtime).collect_CCT)
            key = APROF_(hash)(act->input_size, act->node->context_id);
        else
            key = act->input_size;
        
        Input * tuple = HT_lookup(rtn_info->input_map, key);
        if (tuple == NULL) {
            
            tuple = APROF_(new)(INPUT_S, sizeof(Input));
            
            tuple->min_cumulative_cost = (ULong)-1;
            tuple->min_self_cost = (ULong)-1;
            tuple->key = key;
            
            HT_add_node(rtn_info->input_map, key, tuple);
        }

        tuple->calls++;
        
        // cumulative cost 
        tuple->sum_cumulative_cost += cumul_cost;
        tuple->sqr_cumulative_cost += cumul_cost * cumul_cost;
        
        tuple->max_cumulative_cost = tuple->max_cumulative_cost ^  // max(x, y)
                                    ((tuple->max_cumulative_cost ^ cumul_cost) 
                                    & -(tuple->max_cumulative_cost < cumul_cost)); 
        
        tuple->min_cumulative_cost = tuple->min_cumulative_cost ^ // min(x, y)
                                    ((cumul_cost ^ tuple->min_cumulative_cost) 
                                    & -(cumul_cost < tuple->min_cumulative_cost)); 

        // self cost
        tuple->sum_self_cost += self_cost;
        tuple->sqr_self_cost += self_cost * self_cost;
        
        tuple->max_self_cost = tuple->max_self_cost ^  // max(x, y)
                                    ((tuple->max_self_cost ^ self_cost) 
                                    & -(tuple->max_self_cost < self_cost)); 
        
        tuple->min_self_cost = tuple->min_self_cost ^ // min(x, y)
                                    ((self_cost ^ tuple->min_self_cost) 
                                    & -(self_cost < tuple->min_self_cost)); 

        // real cumulative cost
        if (rtn_info->recursion_pending < 2) 
            tuple->sum_cumul_real_cost += cumul_cost;
        
        #if INPUT_STATS
        
        APROF_(debug_assert)(act->cumul_syscall + act->cumul_thread
                                <= act->input_size, "Wrong");
        
        APROF_(debug_assert)(act->self_syscall <= act->cumul_syscall, "Wrong");
        APROF_(debug_assert)(act->self_thread <= act->cumul_thread, "Wrong");
        
        tuple->sum_cumul_syscall += act->cumul_syscall;
        tuple->sum_cumul_thread  += act->cumul_thread;
        tuple->sum_self_syscall += act->self_syscall;
        tuple->sum_self_thread  += act->self_thread;
        
        #endif // INPUT_STATS   
    }
    
    Activation * parent_activation = APROF_(get_activation_noresize)(
                                tdata, tdata->stack_depth - 1);
    APROF_(debug_assert)(parent_activation != NULL, "Invalid parent activation");

    parent_activation->input_size += act->input_size;
    parent_activation->sum_children_cost += cumul_cost;
    
    #if INPUT_STATS
    parent_activation->cumul_syscall += act->cumul_syscall;
    parent_activation->cumul_thread += act->cumul_thread;
    #endif
}
