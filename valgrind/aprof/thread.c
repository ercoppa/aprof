/*
 * Thread event handlers (start, exit, running)
 * 
 * Last changed: $Date: 2013-09-05 21:30:23 +0200 (gio, 05 set 2013) $
 * Revision:     $Rev: 890 $
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

static ThreadData * APROF_(thread_start)(ThreadId tid){

    APROF_(verbose)(1, "start thread %d\n", tid);
    APROF_(debug_assert)(tid - 1 < VG_N_THREADS && tid > 0, "Invalid tid");
    APROF_(debug_assert)(APROF_(runtime).threads[tid-1] == NULL, "Double thread");
    
    ThreadData * tdata = APROF_(new)(T_S, sizeof(ThreadData));

    tdata->stack_depth = 0;
    tdata->max_stack_size = STACK_SIZE;
    tdata->stack = VG_(calloc)("stack", STACK_SIZE * sizeof(Activation), 1);
    
    #if DEBUG_ALLOCATION
    UInt j = 0;
    for (j = 0; j < STACK_SIZE; j++) APROF_(add_alloc)(ACT_S);
    #endif
    
    tdata->cost = APROF_(time)(tdata);
    tdata->last_bb = NULL;
    tdata->last_exit = NONE;
    tdata->next_activation_id = 1;
    tdata->shadow_memory = LK_create();
    tdata->rtn_ht = HT_construct(VG_(free));
    tdata->next_routine_id = 0;
    tdata->next_context_id = 1;
    
    if (APROF_(runtime).collect_CCT){
        if (!APROF_(runtime).single_report) 
            tdata->root = APROF_(new)(CCT_S, sizeof(CCTNode));
        else
            tdata->root = APROF_(runtime).root;
    }
    
    // registering thread
    APROF_(runtime).threads[tid-1] = tdata;
    APROF_(runtime).running_threads++;
    
    return tdata;
}

void APROF_(thread_exit)(ThreadId tid) {

    APROF_(verbose)(1, "Exit thread %d\n", tid);
    APROF_(debug_assert)(tid - 1 < VG_N_THREADS && tid > 0, "Invalid tid");
    
    ThreadData * tdata = APROF_(runtime).threads[tid - 1];
    APROF_(debug_assert)(tdata != NULL, "Invalid tdata");

    APROF_(unwind_stack)(tdata);
    
    #if !EMPTY_FN_ANALYSIS || !EMPTY_MEM_ANALYSIS
    APROF_(generate_report)(tdata, tid);
    #endif

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
    #endif // EVENTCOUNT

    #if DEBUG_ALLOCATION
    UInt i;
    for (i = 0; i < tdata->max_stack_size; i++) 
        APROF_(remove_alloc)(ACT_S);
    #endif // DEBUG_ALLOCATION

    if (APROF_(runtime).collect_CCT)
        APROF_(free_CCT)(tdata->root);

    VG_(free)(tdata->stack);
    LK_destroy(tdata->shadow_memory);
    HT_destruct(tdata->rtn_ht);
    APROF_(delete)(T_S, tdata);
    
    // Unregistering thread
    APROF_(runtime).threads[tid -1] = NULL;
    APROF_(runtime).current_TID = VG_INVALID_THREADID;
    APROF_(runtime).current_tdata = NULL;
    APROF_(runtime).running_threads--;
}

void APROF_(thread_create)(ThreadId tid, ThreadId child) {
    APROF_(thread_start)(child);
} 

void APROF_(thread_switch)(ThreadId tid, ULong blocks_dispatched) {
    
    /* 
     * Note: Callgrind says that if you have not done at least
     * 5000 blocks you can ignore this call.
        -----
        static ULong last_blocks_done = 0;
        // throttle calls to CLG_(run_thread) by number of BBs executed
        if (blocks_done - blocks_dispatched < 5000) return;
        ------
     * Why?
     */
     
    APROF_(debug_assert)(tid == VG_(get_running_tid)(), "TID mismatch");
    
    if (tid == APROF_(runtime).current_TID) return;
    
    /* save last exit of the previous thread */
    if (APROF_(runtime).current_tdata != NULL)
        APROF_(runtime).current_tdata->last_exit = APROF_(runtime).last_exit;
    
    APROF_(runtime).current_TID = tid;
    if (tid == VG_INVALID_THREADID) {
        APROF_(runtime).current_tdata = NULL;
        return;
    }
    
    if (APROF_(runtime).input_metric == DRMS) 
        APROF_(increase_global_counter)();
    
    if (APROF_(runtime).threads[tid-1] == NULL) {
    
        APROF_(runtime).current_tdata = APROF_(thread_start)(tid);
        APROF_(debug_assert)(APROF_(runtime).current_tdata != NULL, "Invalid tdata");
    
    } else {
    
        APROF_(runtime).current_tdata = APROF_(runtime).threads[tid -1];
        APROF_(verbose)(1, "switch to thread %d\n", tid);
        
    }
    
    /* restore exit value of the current thread */
    APROF_(runtime).last_exit = APROF_(runtime).current_tdata->last_exit;
}

void APROF_(kill_threads)(void) {
    
    UInt i = 0;
    while (i < VG_N_THREADS) {
        
        if (APROF_(runtime).threads[i] != NULL)
            APROF_(thread_exit)(i+1);
        
        i++;
    }
}
