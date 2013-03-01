/*
 * Load, store and modify handlers
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
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

/* Memory resolution: we can aggregate addresses in order
 * to decrese the shadow memory. 
 * - 1 => finest resolution, each byte has its timestamp
 * - 2 => every 2 bytes we have a single timestamp 
 * - ...
 */
UInt APROF_(addr_multiple) = 4;

#if INPUT_METRIC == RVMS
/* 
 * Global shadow memory used for checking latest "version" of an input.
 * If a routine writes a memory cell, we update the associated timestamp
 * in the global shadow memory with the current value of the global counter 
 */
LookupTable * APROF_(global_shadow_memory) = NULL;

/*
 * This global counter is increased by one when there is:
 * - a new routine activation
 * - a thread switch
 */
UInt APROF_(global_counter) = 1;
#endif

VG_REGPARM(3) void APROF_(trace_access)(UWord type, 
                                        Addr addr, 
                                        SizeT size,
                                        UWord kernel_access) {
    
    #if DEBUG
    AP_ASSERT(APROF_(current_TID) == VG_(get_running_tid)(), "TID mismatch");
    #endif
    
    ThreadData * tdata = APROF_(current_tdata);
    #if DEBUG
    AP_ASSERT(tdata != NULL, "Thread data is NULL");
    #endif
    
    #if VERBOSE > 1
    if (type == LOAD) VG_(printf)("Load: %lu:%lu\n", addr, size);
    else if (type == STORE) VG_(printf)("Store: %lu:%lu\n", addr, size);
    else if (type == MODIFY) VG_(printf)("Modify: %lu:%lu\n", addr, size);
    #endif
    
    #if EVENTCOUNT == 1 || EVENTCOUNT == 3
    if (type == LOAD) tdata->num_read++;
    else if (type == STORE) tdata->num_write++;
    else if (type == MODIFY) tdata->num_modify++;
    else AP_ASSERT(0, "Invalid type")
    #endif
    
    #if EMPTY_ANALYSIS
    return;
    #endif
    
    if (tdata->stack_depth == 0) return;
    
    #if TRACE_FUNCTION && IGNORE_DL_RUNTIME
    if (tdata->skip) return;
    #endif
    
    #if COSTANT_MEM_ACCESS
    addr = addr & ~(APROF_(addr_multiple)-1);
    #else
    Int size_fix = size;
    APROF_(fix_access_size)(addr, size_fix);
    #endif
 
    #if !COSTANT_MEM_ACCESS
    while (size_fix > 0) {
    #endif
        
        /*
        UInt j = (addr & 0xffff) >> 2;
        UInt i = addr >> 30; // 14 + 16
        UInt k = (addr >> 16) & 0x3fff;
        VG_(umsg)("addr: %u:%u:%u\n", i, k, j);
        */
        
        #if DEBUG_DRMS
        Bool inc_rms = False;
        Activation * dec_rms = NULL;
        #endif
        
        #if INPUT_METRIC == RMS || DEBUG_DRMS
        Activation * act = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
        
        #if IGNORE_LOAD_SYS
        if (!kernel_access) {
        #else
        if (!kernel_access || type != STORE) {
        #endif

            UInt old_aid = LK_insert(tdata->accesses_rms, addr, act->aid_rms);
            if (old_aid < act->aid_rms && (type == LOAD || type == MODIFY)) {
                
                #if DEBUG_DRMS
                inc_rms = True;
                //VG_(umsg)("RMS++\n");
                #endif
                act->rms++;
                
                if (old_aid > 0 && 
                    old_aid >= APROF_(get_activation_noresize)(tdata, 1)->aid_rms) {
                    
                    Activation * a = APROF_(get_activation_by_aid_rms)(tdata, old_aid);
                    a->rms--;
                    
                    #if DEBUG_DRMS
                    dec_rms = a;
                    #endif

                }

            }
            //VG_(umsg)("old_aid %u - act->aid_rms: %u\n", 
            //                old_aid, act->aid_rms);

        } 
        #endif // INPUT_METRIC == RMS || DEBUG_DRMS
        
        #if INPUT_METRIC == RVMS
        
        UInt ts = APROF_(global_counter);
        UInt old_ts;
        UInt wts = 0;
        
        /*
         * We are writing a new input value. So, this is a new "version"
         * of the memory cell. Update its timestamp in the global shadow memory.
         */
         if (type == STORE || type == MODIFY) {
            
            #if THREAD_INPUT == 1
            wts = LK_insert(APROF_(global_shadow_memory), addr, ts);
            #endif

            if (kernel_access) {
                
                #if THREAD_INPUT == 0
                wts = LK_insert(APROF_(global_shadow_memory), addr, ts);
                #endif
                           
                #if DEBUG_DRMS
                AP_ASSERT(!inc_rms, "Invalid rvms/d_rms [1]");
                AP_ASSERT(dec_rms == NULL, "Invalid d_rms [2]");
                #endif
                                
                #if !COSTANT_MEM_ACCESS
                size_fix -= APROF_(addr_multiple);
                addr += APROF_(addr_multiple);
                continue;
                #else
                return;
                #endif
            }

        } else { 
            
            /*
             * This is a load. What is the latest "version" of this
             * memory cell? Get timestamp of the memory cell in
             * the global shadow memory.
             */
            wts = LK_lookup(APROF_(global_shadow_memory), addr);

        }

        /*
         * Update the timestamp in the private shadow memory.
         */
        old_ts = LK_insert(tdata->accesses_rvms, addr, ts);

        if(type == STORE) {
            
            #if DEBUG_DRMS
            AP_ASSERT(!inc_rms, "Invalid rvms/d_rms [3]");
            AP_ASSERT(dec_rms == NULL, "Invalid d_rms [4]");
            #endif
            
            #if COSTANT_MEM_ACCESS
            return;
            #else
            size_fix -= APROF_(addr_multiple);
            addr += APROF_(addr_multiple);
            continue;
            #endif
        }

        #if DEBUG_DRMS == 0
        Activation * act = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
        #endif
        
        #if DEBUG_DRMS
        Bool inc_rvms = False;
        Bool dec_rvms = False;
        #endif
       
                
        //VG_(umsg)("old_ts %u - act->aid_rvms: %u\n", old_ts, act->aid_rvms);
       
        if(old_ts < wts){
            act->rvms++;
            #if DEBUG_DRMS
            //VG_(umsg)("RVMS++ [1]\n");
            inc_rvms = True;
            #endif
        }
 
        else if (old_ts < act->aid_rvms) {
            
            act->rvms++;
            #if DEBUG_DRMS
            //VG_(umsg)("RVMS++ [2]\n");
            inc_rvms = True;
            #endif
            if (old_ts > 0 && old_ts >= APROF_(get_activation_noresize)(tdata, 1)->aid_rvms) {
                
                APROF_(get_activation_by_aid_rvms)(tdata, old_ts)->rvms--;
                #if DEBUG_DRMS
                dec_rvms = True;
                #endif
            }

        }
        
        #if DISTINCT_RMS
        if (old_ts < act->aid_rvms) {
            
            act->d_rms++;
            #if DEBUG_DRMS
            //VG_(umsg)("D_RMS++\n");
            AP_ASSERT(inc_rms, "Invalid d_rms");
            AP_ASSERT(inc_rvms, "Invalid d_rms");
            #endif
            if (old_ts > 0 && old_ts >= APROF_(get_activation_noresize)(tdata, 1)->aid_rvms) {
                
                Activation * a = APROF_(get_activation_by_aid_rvms)(tdata, old_ts);
                a->d_rms--;
                #if DEBUG_DRMS
                AP_ASSERT(dec_rms == a, "Invalid d_rms");
                #endif
            } 
            #if DEBUG_DRMS
            else {
                AP_ASSERT(!dec_rvms, "Invalid d_rms [3]");
                AP_ASSERT(dec_rms == NULL, "Invalid d_rms");
            }
            #endif
        } 
        #if DEBUG_DRMS
        else {
            AP_ASSERT(!inc_rms, "Invalid d_rms [4]");
            AP_ASSERT(dec_rms == NULL && !inc_rms, "Invalid d_rms");
        }
        #endif
        
        #endif

        #if DEBUG_DRMS
        AP_ASSERT(act->rms == act->d_rms, "Invalid d_rms");
        AP_ASSERT(act->d_rms <= act->rvms, "Invalid RVMS");
        #endif
        
        #endif
    
    #if !COSTANT_MEM_ACCESS
    
        size_fix -= APROF_(addr_multiple);
        addr += APROF_(addr_multiple);
        
    }
    #endif

}
