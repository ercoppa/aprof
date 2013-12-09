/*
 * Load, store and modify handlers
 * 
 * Last changed: $Date: 2013-08-31 10:59:34 +0200 (sab, 31 ago 2013) $
 * Revision:     $Rev: 888 $
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

VG_REGPARM(3) void APROF_(trace_access_rms)(UWord type, 
                                            Addr addr, 
                                            SizeT size) {

    APROF_(verbose)(2, "Load: %lu:%lu\n", addr, size);
    APROF_(verbose)(2, "Store: %lu:%lu\n", addr, size);
    APROF_(verbose)(2, "Modify: %lu:%lu\n", addr, size);

    ThreadData * tdata = APROF_(runtime).current_tdata;
    APROF_(debug_assert)(tdata != NULL, "Thread data is NULL");

    #if EMPTY_MEM_ANALYSIS
    return;
    #endif
    
    #if EVENTCOUNT > 0
    if (type == LOAD) tdata->num_read++;
    else if (type == STORE) tdata->num_write++;
    else if (type == MODIFY) tdata->num_modify++;
    #endif

    if (tdata->stack_depth == 0 || tdata->skip > 0) return;
    
    Int size_fix = size;
    APROF_(fix_access_size)(addr, size_fix);
    
    Activation * act = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
    APROF_(debug_assert)(act != NULL, "act is NULL");
    
    while (size_fix > 0) {
    
        UInt old_aid = LK_insert(tdata->shadow_memory, addr, act->activation_id);
        if (type != STORE && old_aid < act->activation_id) {
            
            act->input_size++;    
            if (old_aid >= APROF_(get_activation_noresize)(tdata, 1)->activation_id) {
                
                Activation * a = APROF_(get_activation_by_aid)(tdata, old_aid);
                a->input_size--;
            }
        }
        
        size_fix -= APROF_(runtime).memory_resolution;
        addr += APROF_(runtime).memory_resolution;
    }
}

VG_REGPARM(3) void APROF_(trace_access_drms)(UWord type, 
                                                Addr addr, 
                                                SizeT size,
                                                UWord kernel_access) {
    
    APROF_(verbose)(2, "Load: %lu:%lu\n", addr, size);
    APROF_(verbose)(2, "Store: %lu:%lu\n", addr, size);
    APROF_(verbose)(2, "Modify: %lu:%lu\n", addr, size);
    
    APROF_(debug_assert)(APROF_(runtime).current_TID 
                            == VG_(get_running_tid)(), "TID mismatch");

    ThreadData * tdata = APROF_(runtime).current_tdata;
    APROF_(debug_assert)(tdata != NULL, "Thread data is NULL");
    
    #if EMPTY_MEM_ANALYSIS
    return;
    #endif

    #if EVENTCOUNT > 0
    if (type == LOAD) tdata->num_read++;
    else if (type == STORE) tdata->num_write++;
    else if (type == MODIFY) tdata->num_modify++;
    #endif
    
    if (tdata->stack_depth == 0 || tdata->skip > 0) return;
    
    Int size_fix = size;
    APROF_(fix_access_size)(addr, size_fix);
 
    while (size_fix > 0) {

        UInt ts = APROF_(runtime).global_counter;
        UInt old_ts;
        UInt wts = 0;
        
        /*
         * We are writing a new input value. So, this is a new "version"
         * of the memory cell. Update its timestamp in the global shadow memory.
         */
        switch(type) {
             
            case STORE:
            case MODIFY: // LOAD+STORE
            
                #if INPUT_STATS
                if (kernel_access)
                    wts = LK_insert(APROF_(runtime).global_shadow_memory, addr, SET_SYSCALL(ts));
                else // 31th bit is already cleared, avoid SET_THREAD
                    wts = LK_insert(APROF_(runtime).global_shadow_memory, addr, ts);
                #else // INPUT_STATS
                wts = LK_insert(APROF_(runtime).global_shadow_memory, addr, ts);
                #endif // INPUT_STATS
                        
                break;
            
            case LOAD:
            
                /*
                 * This is a load. What is the latest "version" of this
                 * memory cell? Get timestamp of the memory cell in
                 * the global shadow memory.
                 */
                wts = LK_lookup(APROF_(runtime).global_shadow_memory, addr);
            
                break;
            
            default:
                APROF_(assert)(0, "Invalid memory event");
        }

        if (type == STORE) {
            
            size_fix -= APROF_(runtime).memory_resolution;
            addr += APROF_(runtime).memory_resolution;
            continue;
        }
        
        /*
         * Update the timestamp in the private shadow memory.
         */
        old_ts = LK_insert(tdata->shadow_memory, addr, ts);

        Activation * act = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
        
        // Dynamic input?
        if(old_ts < TS(wts)){
            
            act->input_size++;
            
            #if INPUT_STATS
            if (SYSCALL(wts)) {
                act->self_syscall++;
                act->cumul_syscall++;
            } else {
                act->self_thread++;
                act->cumul_thread++;
            }
            APROF_(debug_assert)(act->cumul_thread + act->cumul_syscall 
                                    <= act->input_size, "Wrong");
            #endif // INPUT_STATS
        }
 
        // RMS?
        else if (old_ts < act->activation_id) {
            
            act->input_size++;
            if (old_ts > 0 && old_ts >= APROF_(get_activation_noresize)(tdata, 1)->activation_id) {
                
                APROF_(get_activation_by_aid)(tdata, old_ts)->input_size--;
            }
        }
    
        size_fix -= APROF_(runtime).memory_resolution;
        addr += APROF_(runtime).memory_resolution;
    }
}
