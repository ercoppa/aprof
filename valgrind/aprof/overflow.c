/*
 * Global/local counter overflow handlers
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

#define OVERFLOW_DEBUG 0

#if INPUT_METRIC == DRMS
/*
 * global_counter is 32 bit integer so it can overflow. To overcome this
 * issue we periodically compress our set of valid timestamps 
 * (e.g., after an overflow).
 * 
 * 1) scan all shadow thread stacks (in parallel): collect the (sorted) 
 *    set of used timestamps by all activations (active_ts).
 * 
 * 3) element i-th of the set is associated to the new timestamp 
 *    i * 3. Re-assign ts to all activations.
 *
 * 4) re-assign ts in all shadow memories (see LK_compress)
 * 
 * Return the new starting value for the global counter: 
 *      3 * size(active_ts).
 */
#if OVERFLOW_DEBUG
static UInt round = 0;
#endif
UInt APROF_(overflow_handler_drms)(void) {

    //VG_(umsg)("Global counter overflow\n");

    #if OVERFLOW_DEBUG
    HChar name[128];
    VG_(sprintf)(name, "overflow_log_%u", round++);
    FILE * f = APROF_(fopen)(name);
    if (f == NULL) AP_ASSERT(0, "Cannot open overflow log");
    #endif
    
    UInt sum = 0; // # valid timestamps
    UInt count_thread = APROF_(running_threads);

    // pointers to active shadow memories
    LookupTable ** shamem = VG_(calloc)("pointers to all PSM", 
                                            count_thread, 
                                            sizeof(LookupTable *));
    
    // current stack depth of each thread
    UInt * stack_depths = VG_(calloc)("index for merge", 
                                    count_thread, sizeof(UInt));
    
    #if OVERFLOW_DEBUG
    VG_(umsg)("Estimating # valid ts\n");
    #endif
    
    UInt i = 0;
    UInt j = 0;
    // compute the number of different activation-ts
    while(i < count_thread && j < VG_N_THREADS){
        
        if(threads[j] == NULL) {
        
            j++;
        
        } else {
        
            #if OVERFLOW_DEBUG
            APROF_(fprintf)(f, "Thread: %u ~ depth: %d\n", j, 
                                    threads[j]->stack_depth);
            APROF_(fflush)(f);
            #endif
        
            shamem[i] = threads[j]->accesses_rvms;
            stack_depths[i] = threads[j]->stack_depth;
            sum += stack_depths[i]; // over-estimation
            i++; j++;
        
        }
    }
    
    #if OVERFLOW_DEBUG
    VG_(umsg)("Collecting valid ts & stack re-assignment...\n");
    #endif
 
    // current valid timestamps 
    sum++; // in order to have an initiali zero
    UInt * active_ts = VG_(calloc)("array overflow", sum, sizeof(UInt));
    
    /* 
     * Collect valid activation-ts using a merge 
     * and re-assign the new ts to every shadow stacks.
     * 
     * stack_depths[i] contains the lower activation (of the shadow
     * stack for i-th thread) already checked as candidate
     * for the current max. Basically, we find the max ts on
     * top of all shadow stacks, we store this ts in active_ts,
     * we decrease the stack_depths[i] of i-th thread (the one
     * with max_act), then we find the new max ts (it can be on 
     * top of another shadow stack).
     * 
     */
    for(i = (sum - 1); i > 0; i--){
        
        UInt k = 0;
        UInt max = 0; 
        Activation * act_tmp;
        
        /*
         * Info about activation/thread with the max activation-ts
         * 
         *  act_max: the current activation with the higher ts
         *  max_ind: this is the index in stack_depths[] related
         *           to the thread with highest ts (act_max).
         */
        Activation * act_max = NULL;
        UInt max_ind = 0;

        /* find the max activation ts among all shadow stacks */ 
        for(j = 0; j < count_thread; j++){

            while(threads[k] == NULL) k++;

            if(stack_depths[j] > 0){
                
                act_tmp = APROF_(get_activation)(threads[k], stack_depths[j]);
                
                #if OVERFLOW_DEBUG
                APROF_(fprintf)(f, "Thread[%u]: aid = %u\n", k, act_tmp->aid_rvms);
                APROF_(fflush)(f);
                #endif

                if(max < act_tmp->aid_rvms){
                    
                    max = act_tmp->aid_rvms;
                    act_max = act_tmp;
                    max_ind = j;
                
                }
            
            }
            k++;
    
        }

        #if OVERFLOW_DEBUG
        APROF_(fprintf)(f, "Max: %u\n", max);
        APROF_(fflush)(f);
        #endif

        active_ts[i] = max;
        
        // next time we check for the max the caller of this act
        stack_depths[max_ind]--; 
        act_max->aid_rvms = i*3; // re-assign ts
    
    }
    
    #if OVERFLOW_DEBUG
    VG_(umsg)("ts collected...\n");
    APROF_(fprintf)(f, "ACTIVE_TS: ");
    APROF_(fflush)(f);
    for (i = 0; i < sum; i++) {
        APROF_(fprintf)(f, " %u=%u ", i, active_ts[i]);
        APROF_(fflush)(f);
    }
    APROF_(fprintf)(f, "\n");
    APROF_(fflush)(f);
    #endif
    
    VG_(free)(stack_depths);

    // compress shadow memories
    #if OVERFLOW_DEBUG
    VG_(umsg)("Compressing\n");
    LK_compress(active_ts, sum, shamem, f);
    #else
    LK_compress(active_ts, sum, shamem);
    #endif
    
    VG_(free)(active_ts);
    VG_(free)(shamem);
    
    #if OVERFLOW_DEBUG
    APROF_(fclose)(f);
    #endif
    //VG_(umsg)("Global counter overflow handler end\n");
    //AP_ASSERT(0, "test");
    
    return 3 * (sum + 1);
}
#endif

#if INPUT_METRIC == RMS || DEBUG_DRMS
UInt APROF_(overflow_handler_rms)(void) {

    #if DEBUG
    VG_(umsg)("Local counter overflow\n");
    #endif

    ThreadData * tdata = APROF_(current_tdata);
    #if DEBUG
    AP_ASSERT(tdata != NULL, "Invalid tdata");
    #endif

    /* Collect all valid aid */
    UInt * arr_aid = VG_(calloc)("arr rid", tdata->stack_depth, sizeof(UInt));
    int j = 0;
    //VG_(umsg)("Stack depth: %d\n", tdata->stack_depth);
    for (j = 0; j < tdata->stack_depth; j++) {
    
        Activation * act_c = APROF_(get_activation)(tdata, j + 1);
        //VG_(umsg)("%u=%u ", j + 1, act_c->aid_rms);
        arr_aid[j] = act_c->aid_rms;
        act_c->aid_rms = j + 1;
        
    }
    
    //VG_(umsg)("\n");
    LK_compress_rms(tdata->accesses_rms, arr_aid, tdata->stack_depth);
    VG_(free)(arr_aid);

    //VG_(umsg)("Local counter: %u\n", tdata->stack_depth + 1);
    return tdata->stack_depth + 1;

}
#endif // INPUT_METRIC == RMS || DISTINCT_RMS
