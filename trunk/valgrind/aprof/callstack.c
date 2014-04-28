/*
 * Simulated stack. aprof can trace functions in two ways:
 * - with an internal tracing mechanism (same approach of callgrind)
 * - through the help of GCC instrumentation (-finstrument-functions)
 *   more info at http://code.google.com/p/aprof/wiki/AprofOnARM_PPC
 * 
 * Last changed: $Date: 2013-02-28 15:23:24 +0100 (gio, 28 feb 2013) $
 * Revision:     $Rev: 811 $
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

Activation * APROF_(resize_stack)(ThreadData * tdata, unsigned int depth) {

    APROF_(debug_assert)(tdata != NULL, "Invalid tdata in get_activation");
    
    /* Safety check, better always run this :) */
    APROF_(assert)(depth > 0 && depth < MAX_STACK_SIZE, "Invalid depth");

    /* Expand stack if necessary */
    if (depth - 1 >= tdata->max_stack_size) {
        
        #if DEBUG_ALLOCATION
        int j = 0;
        for (j = 0; j < tdata->max_stack_size; j++) 
            APROF_(add_alloc)(ACT_S);
        #endif
        
        tdata->max_stack_size = tdata->max_stack_size * 2; 
        
        tdata->stack = VG_(realloc)("stack", tdata->stack, 
                            tdata->max_stack_size * sizeof(Activation));
    }
    
    return tdata->stack + depth - 1;
}

/* Obtain a BB from bb_ht or allocate a new BB */
BB * APROF_(get_BB)(UWord target) {
    
    BB * bb = HT_lookup(APROF_(runtime).bb_ht, target);
    if (bb == NULL) bb = APROF_(new)(BB_S, sizeof(BB));    
    return bb;
}

/* 
 * search_runtime_resolve() and check_code are 
 * completly taken from callgrind!
 */

/* 
 * Scan for a pattern in the code of an ELF object.
 * If found, return true and set runtime_resolve_{addr,length}
 */
__attribute__((unused))    // Possibly;  depends on the platform.
static Bool check_code(UWord obj_start, 
                        UWord obj_size, 
                        UChar code[], 
                        struct pattern* pat) {
                            
    Bool found;
    Addr addr, end;
    int chunk, start, len;

    /* 
     * first chunk of pattern should always start at offset 0 and
     * have at least 3 bytes 
     */
    APROF_(assert)((pat->chunk[0].start == 0) && (pat->chunk[0].len >2), 
                                            "Check on chunk pat failed");

    end = obj_start + obj_size - pat->len;
    addr = obj_start;
    while(addr < end) {
        
        found = (VG_(memcmp)( (void*)addr, code, pat->chunk[0].len) == 0);
        if (found) {
            
            chunk = 1;
            while(1) {
                start = pat->chunk[chunk].start;
                len   = pat->chunk[chunk].len;
                if (len == 0) break;

                APROF_(assert)(len >2, "fail check pattern");

                if (VG_(memcmp)( (void*)(addr+start), code+start, len) != 0) {
                    found = False;
                    break;
                }
                chunk++;
            }

            if (found) {
                
                APROF_(runtime).dl_runtime_resolve_addr = addr;
                APROF_(runtime).dl_runtime_resolve_length = pat->len;
                return True;
            }
        }
        addr++;
    }

    return False;
}


/* _ld_runtime_resolve, located in ld.so, needs special handling:
 * The jump at end into the resolved function should not be
 * represented as a call (as usually done in callgrind with jumps),
 * but as a return + call. Otherwise, the repeated existance of
 * _ld_runtime_resolve in call chains will lead to huge cycles,
 * making the profile almost worthless.
 *
 * If ld.so is stripped, the symbol will not appear. But as this
 * function is handcrafted assembler, we search for it.
 *
 * We stop if the ELF object name does not seem to be the runtime linker
 */
static Bool search_runtime_resolve(const HChar * obj_name, 
                                    UWord obj_start,
                                    UWord obj_size) {
                                        
#if defined(VGP_x86_linux)
    static UChar code[] = {
    /* 0*/ 0x50, 0x51, 0x52, 0x8b, 0x54, 0x24, 0x10, 0x8b,
    /* 8*/ 0x44, 0x24, 0x0c, 0xe8, 0x70, 0x01, 0x00, 0x00,
    /*16*/ 0x5a, 0x59, 0x87, 0x04, 0x24, 0xc2, 0x08, 0x00 };
    /* Check ranges [0-11] and [16-23] ([12-15] is an absolute address) */
    static struct pattern pat = {
    "x86-def", 24, {{ 0,12 }, { 16,8 }, { 24,0}} };

    /* Pattern for glibc-2.8 on OpenSuse11.0 */
    static UChar code_28[] = {
    /* 0*/ 0x50, 0x51, 0x52, 0x8b, 0x54, 0x24, 0x10, 0x8b,
    /* 8*/ 0x44, 0x24, 0x0c, 0xe8, 0x70, 0x01, 0x00, 0x00,
    /*16*/ 0x5a, 0x8b, 0x0c, 0x24, 0x89, 0x04, 0x24, 0x8b,
    /*24*/ 0x44, 0x24, 0x04, 0xc2, 0x0c, 0x00 };
    static struct pattern pat_28 = {
    "x86-glibc2.8", 30, {{ 0,12 }, { 16,14 }, { 30,0}} };

    if (VG_(strncmp)(obj_name, "/lib/ld", 7) != 0) return False;
    if (check_code(obj_start, obj_size, code, &pat)) return True;
    if (check_code(obj_start, obj_size, code_28, &pat_28)) return True;
    return False;
#endif

#if defined(VGP_ppc32_linux)
    static UChar code[] = {
    /* 0*/ 0x94, 0x21, 0xff, 0xc0, 0x90, 0x01, 0x00, 0x0c,
    /* 8*/ 0x90, 0x61, 0x00, 0x10, 0x90, 0x81, 0x00, 0x14,
    /*16*/ 0x7d, 0x83, 0x63, 0x78, 0x90, 0xa1, 0x00, 0x18,
    /*24*/ 0x7d, 0x64, 0x5b, 0x78, 0x90, 0xc1, 0x00, 0x1c,
    /*32*/ 0x7c, 0x08, 0x02, 0xa6, 0x90, 0xe1, 0x00, 0x20,
    /*40*/ 0x90, 0x01, 0x00, 0x30, 0x91, 0x01, 0x00, 0x24,
    /*48*/ 0x7c, 0x00, 0x00, 0x26, 0x91, 0x21, 0x00, 0x28,
    /*56*/ 0x91, 0x41, 0x00, 0x2c, 0x90, 0x01, 0x00, 0x08,
    /*64*/ 0x48, 0x00, 0x02, 0x91, 0x7c, 0x69, 0x03, 0xa6, /* at 64: bl aff0 <fixup> */
    /*72*/ 0x80, 0x01, 0x00, 0x30, 0x81, 0x41, 0x00, 0x2c,
    /*80*/ 0x81, 0x21, 0x00, 0x28, 0x7c, 0x08, 0x03, 0xa6,
    /*88*/ 0x81, 0x01, 0x00, 0x24, 0x80, 0x01, 0x00, 0x08,
    /*96*/ 0x80, 0xe1, 0x00, 0x20, 0x80, 0xc1, 0x00, 0x1c,
    /*104*/0x7c, 0x0f, 0xf1, 0x20, 0x80, 0xa1, 0x00, 0x18,
    /*112*/0x80, 0x81, 0x00, 0x14, 0x80, 0x61, 0x00, 0x10,
    /*120*/0x80, 0x01, 0x00, 0x0c, 0x38, 0x21, 0x00, 0x40,
    /*128*/0x4e, 0x80, 0x04, 0x20 };
    static struct pattern pat = {
    "ppc32-def", 132, {{ 0,65 }, { 68,64 }, { 132,0 }} };

    if (VG_(strncmp)(obj_name, "/lib/ld", 7) != 0) return False;
    return check_code(obj_start, obj_size, code, &pat);
#endif

#if defined(VGP_amd64_linux)
    static UChar code[] = {
    /* 0*/ 0x48, 0x83, 0xec, 0x38, 0x48, 0x89, 0x04, 0x24,
    /* 8*/ 0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x89, 0x54, 0x24, 0x10,
    /*18*/ 0x48, 0x89, 0x74, 0x24, 0x18, 0x48, 0x89, 0x7c, 0x24, 0x20,
    /*28*/ 0x4c, 0x89, 0x44, 0x24, 0x28, 0x4c, 0x89, 0x4c, 0x24, 0x30,
    /*38*/ 0x48, 0x8b, 0x74, 0x24, 0x40, 0x49, 0x89, 0xf3,
    /*46*/ 0x4c, 0x01, 0xde, 0x4c, 0x01, 0xde, 0x48, 0xc1, 0xe6, 0x03,
    /*56*/ 0x48, 0x8b, 0x7c, 0x24, 0x38, 0xe8, 0xee, 0x01, 0x00, 0x00,
    /*66*/ 0x49, 0x89, 0xc3, 0x4c, 0x8b, 0x4c, 0x24, 0x30,
    /*74*/ 0x4c, 0x8b, 0x44, 0x24, 0x28, 0x48, 0x8b, 0x7c, 0x24, 0x20,
    /*84*/ 0x48, 0x8b, 0x74, 0x24, 0x18, 0x48, 0x8b, 0x54, 0x24, 0x10,
    /*94*/ 0x48, 0x8b, 0x4c, 0x24, 0x08, 0x48, 0x8b, 0x04, 0x24,
    /*103*/0x48, 0x83, 0xc4, 0x48, 0x41, 0xff, 0xe3 };
    static struct pattern pat = {
    "amd64-def", 110, {{ 0,62 }, { 66,44 }, { 110,0 }} };

    if ((VG_(strncmp)(obj_name, "/lib/ld", 7) != 0) &&
    (VG_(strncmp)(obj_name, "/lib64/ld", 9) != 0)) return False;
    return check_code(obj_start, obj_size, code, &pat);
#endif

    /* For other platforms, no patterns known */
    return False;
}

/* End callgrind code */

/* Push a new activation on the stack */
static void APROF_(push_stack)(ThreadData * tdata, 
                                UWord sp, 
                                Function * f) {
    
    
    APROF_(debug_assert)(sp > 0, "Invalid stack pointer");
    APROF_(debug_assert)(f != NULL, "Invalid function");
    
    tdata->stack_depth++;
    Activation * act = APROF_(get_activation)(tdata, tdata->stack_depth);
    APROF_(debug_assert)(act != NULL, "Invalid activation info");
    
    RoutineInfo * rtn_info = HT_lookup(tdata->rtn_ht, (UWord) f); 
    if (rtn_info == NULL)
        rtn_info = APROF_(new_routine_info)(tdata->rtn_ht, f, (UWord) f, tdata);
        
    act->sp = sp;
    act->routine_info = rtn_info;
    
    // We change this maybe in the future if this fn calls another fn 
    act->ret_addr = 0; 
    
    APROF_(function_enter)(tdata, act);
}

/* 
 * Sync stack with current SP (CSP), if n_frames > 0
 * and CSP == SP, pop out n_frames activation.
 * Return how many activations are been removed from the stack.
 */
static UInt APROF_(pop_stack)(ThreadData * tdata, 
                                UWord csp, 
                                UInt n_frames,
                                Activation * current) {
    
    APROF_(debug_assert)(csp > 0, "Invalid current stack pointer");
    APROF_(debug_assert)(current != NULL, "Invalid activation"); 
    
    UInt n_pop = 0;
    while ( tdata->stack_depth > 0 && 
            (
                csp > current->sp || 
                (csp == current->sp && n_frames > 0)
            )
          ) 
    {
        APROF_(function_exit)(tdata, current);
        
        if (n_frames > 0) n_frames--;
        n_pop++;
        
        /* Adjust stack */
        APROF_(assert)(tdata->stack_depth > 0, "Invalid stack depth");
        tdata->stack_depth--;
        current--;
    }
    
    return n_pop;
}

/* Helper function called at start of a BB */
VG_REGPARM(2) void APROF_(BB_start)(UWord target, BB * bb) {

    APROF_(debug_assert)(target > 0, "Invalid target");
    APROF_(debug_assert)(bb != NULL, "Invalid BB");

    /* Safety check due to an old bug of Valgrind */
    APROF_(debug_assert)(APROF_(runtime).current_TID 
                            == VG_(get_running_tid)(), "TID mismatch");
                            
    /* Get thread data */
    ThreadData * tdata = APROF_(runtime).current_tdata;
    
    #if COST == BB_COUNT
    tdata->cost++;
    #endif
    
    /* Previous BB executed */
    BB * last_bb = tdata->last_bb;
    enum jump_t exit = NONE;
    if (last_bb != NULL)
        exit = APROF_(runtime).last_exit;

    /* Get current stack pointer */
    UWord csp = (UWord) VG_(get_SP)(APROF_(runtime).current_TID);
    
    /* Create a new BB structure */
    if (bb->key == 0) {
        
        bb->key = target;
        
        /* Obtain section kind of this BB */
        bb->obj_section = VG_(DebugInfo_sect_kind)(NULL, 0, target);
        
        /* Function name buffer */
        HChar * fn = APROF_(new)(FN_NAME_S, NAME_SIZE);
        
        /* 
         * Are we entering a new function? Ask to Valgrind
         * if this BB is the first of a function. Valgrind sometimes
         * does not know this information, so it return False. If this is
         * the case, we try different stategies to capture a call, see
         * later.
         */
        bb->is_entry = VG_(get_fnname_if_entry)(target, fn, NAME_SIZE);
        
        /* If is not entry, we need anyway info about this function */
        Bool info_fn = True; 
        if (!bb->is_entry) {
            info_fn = VG_(get_fnname)(target, fn, NAME_SIZE);
        }

        if (info_fn && VG_(strcmp)(fn, "(below main)") == 0) {
            VG_(sprintf)(fn, "below_main");
        }
        
        if (info_fn && VG_(strcmp)(fn, "_dl_runtime_resolve") == 0) {
            
            bb->is_dl_runtime_resolve = True;
            // this only means that ld is not stripped 
            APROF_(runtime).dl_runtime_resolve_addr = 1;         
        }
        
        Function * f = NULL;
        UInt hash = 0;
        if (info_fn) {
            
            hash = APROF_(str_hash)((HChar *)fn);
            f = HT_lookup(APROF_(runtime).fn_ht, hash);
            
            while (f != NULL && VG_(strcmp)(f->name, fn) != 0) {
                f = f->next;
            }
        }
        
        Object * obj = NULL;
        const HChar * obj_name = NULL;
        if (f == NULL || f->obj == NULL) {
            
            /* Obtain debug info about this BB */
            DebugInfo * di = VG_(find_DebugInfo)(target);
            /* Obtain object name */
            obj_name = di ? VG_(DebugInfo_get_filename)(di) : NULL;
            
            UInt hash_obj = 0;
            if (obj_name != NULL) {
                
                hash_obj = APROF_(str_hash)((HChar *) obj_name);
                obj = HT_lookup(APROF_(runtime).obj_ht, hash_obj);
                
                while (obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0) {
                    obj = obj->next;
                }
                
                if (obj == NULL) {
                    
                    HChar * obj_name_c = VG_(strdup)("obj_name", obj_name);
                    APROF_(add_alloc)(OBJ_NAME_S);
                    
                    obj = APROF_(new)(OBJ_S, sizeof(Object));
                    
                    obj->key = hash_obj;
                    obj->name = obj_name_c;
                    obj->filename = NULL; /* FixMe */
                    HT_add_node(APROF_(runtime).obj_ht, obj->key, obj);
                }
            }
            
            /* try to see if we find dl_runtime_resolve in this obj */
            if (APROF_(runtime).dl_runtime_resolve_addr == 0
                && !bb->is_dl_runtime_resolve && obj_name != NULL
                && last_bb != NULL && obj != last_bb->fn->obj) {
            
                /* Obtain obj start address */
                UWord obj_start = di ? VG_(DebugInfo_get_text_avma)(di) : 0;
                /* Obtaind obj size */
                UWord obj_size = di ? VG_(DebugInfo_get_text_size)(di) : 0;
                /* Check */
                if (search_runtime_resolve(obj_name, obj_start, obj_size)) {
                    
                    bb->is_dl_runtime_resolve = True;
                    VG_(sprintf)(fn, "_dl_runtime_resolve");
                    info_fn = True;
                }
            }
        }
        
        /* Is this BB dl_runtime_resolve? */ 
        if (
            !bb->is_dl_runtime_resolve 
            && APROF_(runtime).dl_runtime_resolve_addr > 1 
            && (target >= APROF_(runtime).dl_runtime_resolve_addr) 
            && (target < (APROF_(runtime).dl_runtime_resolve_addr 
                            + APROF_(runtime).dl_runtime_resolve_length))
            ) 
        {
            bb->is_dl_runtime_resolve = True;
            VG_(sprintf)(fn, "_dl_runtime_resolve");
            info_fn = True;
        }
        
        /* 
         * We don't have info about current fn, but this is something probably
         * interesting anyway (various functions are implemented 
         * directly in assembly, maybe we should get for this also more
         * info about filename/line/dir as callgrind does )
         */
        Bool unknown = False;
        if (!info_fn) {
            
            if (bb->obj_section == Vg_SectPLT)
                VG_(sprintf)(fn, "PLT_%p", (void *) bb->key);
            else 
                VG_(sprintf)(fn, "%p", (void *)bb->key);
                
            info_fn = True;
            unknown = True;
        }
        
        if (info_fn && f == NULL) {
            
            /* Maybe we have renamed the function...*/
            UInt hash_2 = APROF_(str_hash)(fn);
            if (hash != hash_2) {
                
                hash = hash_2;
                f = HT_lookup(APROF_(runtime).fn_ht, hash);
                
                while (f != NULL && VG_(strcmp)(f->name, fn) != 0) {
                    f = f->next;
                }
            }
            
            if (f == NULL) {
                
                f = APROF_(new)(FN_S, sizeof(Function));
                f->key = hash;
                
                /* 
                 * fn is a buffer of 4096, if possible try to minimize
                 * the wasted space
                 */
                HChar * fn_c = VG_(strdup)("fn_name", fn);
                VG_(free)(fn);
                fn = fn_c;
                
                f->name = fn;
                
                if (unknown) f->discard = True;
                else {
                    
                    if (APROF_(runtime).single_report)
                        f->input_map = HT_construct(VG_(free));
                    
                    f->function_id = APROF_(runtime).next_function_id++;
                    
                    HChar * mangled = APROF_(new)(MANGLED_S, NAME_SIZE);
                    if(VG_(get_fnname_no_cxx_demangle)(bb->key, (Char *) mangled, NAME_SIZE)) {
                        
                        if (VG_(strcmp)(mangled, "(below main)") == 0) {
                            VG_(sprintf)(mangled, "below_main");
                        }

                        if (VG_(strcmp)(f->name, mangled) == 0) {
                            APROF_(delete)(MANGLED_S, mangled); 
                        } else {
                            
                            /* try min wasted space */
                            char * mangled_c = VG_(strdup)("mangle_name", mangled);
                            VG_(free)(mangled);
                            mangled = mangled_c;
                            f->mangled = mangled;
                        }
                    
                    } else 
                        APROF_(delete)(MANGLED_S, mangled);
                }
                
                f->obj = obj;
                HT_add_node(APROF_(runtime).fn_ht, f->key, f);

            } else
                APROF_(delete)(FN_NAME_S, fn);
            
        } else 
            APROF_(delete)(FN_NAME_S, fn);
        
        #if SKIP_DL_RUNTIME
        if (bb->is_dl_runtime_resolve) 
            f->skip = True;
        #endif
        
        bb->fn = f;
        bb->instr_offset = 0; /* real value filled with a store */
        HT_add_node(APROF_(runtime).bb_ht, bb->key, bb);
    }

    /* Are we converting RET into a CALL? */
    Bool ret_as_call = False;
    
    /* Estimation of number of returned functions */ 
    UInt n_pop = 1;
    
    /* Current activation */
    Activation * current = NULL;
    if (tdata->stack_depth > 0) 
        current = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
    
    /* 
     * If last BB does an Ijk_Ret exit not always this is a real return.
     * From setup_bbcc of callgrind: 
     * -----
     * We have a real return if
     * - the stack pointer (SP) left the stack. stack frame, or
     * - SP has the same value as when reaching the stack. function
     *   and the address of this BB is the return address of last call
     *   (we even allow to leave multiple frames if the SP stays the
     *    same and we find a matching return address)
     * The latter condition is needed because on PPC, SP can stay
     * the same over CALL=b(c)l / RET=b(c)lr boundaries
     * -----
     * If it's not a real return, then we can simulate a call 
     * (pattern: PUSH addr; RET; this is only another way of 
     * saying "JMP addr").
     */
    if (exit == BBRET) {
        
        /* This is a call! */
        if (csp < current->sp) {
            n_pop = 0;
        }
        
        /* 
         * SP does not change, so this is a call only if return 
         * does not match our expected return address.
         */
        else if (csp == current->sp) {
            
            Activation * c = current;
            UInt depth = tdata->stack_depth;
            while(c != NULL) {
                
                if (c->ret_addr == target) break;
                if (depth > 0) {
                    
                    depth--;
                    c--;
                    
                    if (c->sp == csp) {
                        n_pop++;
                        continue; 
                    }
                
                }
                /* 
                 * We never find a match btw expected ret_addr and
                 * current bb; we don't know what do, delete all
                 * planned pop operations.
                 */
                n_pop = 0;
                break;
            }
            
        }
        
        if (n_pop == 0) {
            ret_as_call = True;
            exit = BBOTHER;
        }
    }
    
    /* This is a call also if last BB exit is
     * not Ijk_Call/Ijk_Ret and:
     * - current BB is in different ELF object or section
     * - current BB is first BB of a function (we know this from
     *   previous info given by Valgrind) 
     */
    Bool call_emulation = False;
    if (last_bb != NULL && exit != BBCALL && exit != BBRET) {
        
        Bool different_obj = False;
        if (last_bb != NULL && bb->fn->obj != last_bb->fn->obj)
            different_obj = True;

        Bool different_sect = False;
        if (last_bb != NULL && bb->obj_section != last_bb->obj_section)
            different_sect = True;
        
        if (ret_as_call || different_obj || different_sect || bb->is_entry) {
            
            /* 
             * if the last BB is dl_runtime_resolve, we have to 
             * do a pop in out stack. If we avoid this, then
             * we see that dl_runtime_resolve call the resolved
             * function (this not make sense! It resolve and
             * return to the caller).
             */
            if (tdata->stack_depth > 0 && last_bb->is_dl_runtime_resolve) {
                
                /* Update current stack pointer */
                csp = current->sp;
                APROF_(function_exit)(tdata, current);
                
                /* Adjust stack */
                APROF_(debug_assert)(tdata->stack_depth > 0, "Stack depth not possible");
                tdata->stack_depth--;
                current = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
            }
            
            exit = BBCALL;
            
            /* 
             * We are simulating a call... later (maybe) we have to fix 
             * something in the stack
             */
            call_emulation = True;
        }
    }
    
    /* 
     * Ok, now it's time to adjust stack and notify function exit or
     * entry :)
     */
    if (exit == BBRET) {
        
        APROF_(pop_stack)(tdata, csp, n_pop, current);
        
    } else {
        
        /* 
         * By definition, if this is a call, SP must be equal or smaller
         * of SP on top of our stack. Try to see if this is not true...
         */
        if (tdata->stack_depth > 0) {
            
            n_pop = APROF_(pop_stack)(tdata, csp, 0, current);
            if (n_pop > 0) {
                exit = BBRET;
            } 
        }
        
        if (exit == BBCALL) {
            
            /* I don't know why we do this but callgrind does so... */
            if (call_emulation && tdata->stack_depth > 0)
                csp = current->sp;
            
            if (tdata->stack_depth > 0 && last_bb != NULL && last_bb->exit == BBCALL) {
                current->ret_addr = last_bb->key + last_bb->instr_offset;
            }
            
            APROF_(push_stack)(tdata, csp, bb->fn); 
        }
    }
    
    /* Update pointer last BB executed */
    tdata->last_bb = bb;
    
    /* Reset exit of current BB */
    APROF_(runtime).last_exit = BBOTHER;
}

void APROF_(unwind_stack)(ThreadData * tdata) {
    
    while (tdata->stack_depth > 0)  {

        Activation * current = APROF_(get_activation_noresize)(tdata, tdata->stack_depth);
        APROF_(function_exit)(tdata, current);
        tdata->stack_depth--;
        current--;
    }
}

Bool APROF_(trace_function)(ThreadId tid, UWord * arg, UWord * ret) {

#if 0
    /* Is this client request for aprof? */
    if (!VG_IS_TOOL_USERREQ('V','A',arg[0])) return False;
    
    #if EMPTY_ANALYSIS
    return True;
    #endif
    
    ThreadData * tdata = APROF_(runtime).current_tdata;
    #if DEBUG
    APROF_(assert)(tdata != NULL, "Invalid tdata");
    #endif
    
    UWord target = arg[1]; 
    #if DEBUG
    APROF_(assert)(target > 0, "Invalid target")
    #endif
    
    if (arg[2] == 1) {
        
        tdata->stack_depth++;
        Activation * act = APROF_(get_activation)(tdata, tdata->stack_depth);
        #if DEBUG
        APROF_(assert)(act != NULL, "Invalid activation info");
        #endif
        
        char * rtn_name = VG_(calloc)("rtn_name", NAME_SIZE, 1);
        #if DEBUG
        APROF_(assert)(rtn_name != NULL, "rtn_name not allocable");
        #endif
        
        #if DEBUG_ALLOCATION
        APROF_(add_alloc)(FN_NAME);
        #endif
        
        if (!VG_(get_fnname)(target, rtn_name, NAME_SIZE))
            VG_(sprintf)(rtn_name, "%p", (void *) target);
        
        Function * fn = NULL;
        fn = HT_lookup(APROF_(runtime).fn_ht, target);
        
        RoutineInfo * rtn_info = NULL;
        if (fn != NULL) {
            rtn_info = HT_lookup(tdata->routine_hash_table, target); 
        }
        if (rtn_info == NULL) {
            
            if (fn == NULL) {
                
                fn = VG_(calloc)("fn node", sizeof(Function), 1);
                #if DEBUG
                APROF_(assert)(fn != NULL, "fn node not allocable");
                #endif
                
                #if DEBUG_ALLOCATION
                APROF_(add_alloc)(FNS);
                #endif
                
                char * mangled = VG_(calloc)("mangled", NAME_SIZE, 1);
                #if DEBUG
                APROF_(assert)(mangled != NULL, "mangled name not allocable");
                #endif
                if(    VG_(get_fnname_no_cxx_demangle)(target, mangled, NAME_SIZE)
                    && VG_(strcmp)(rtn_name, mangled) != 0
                    ) {
                    fn->mangled = mangled;
                    
                    #if DEBUG_ALLOCATION
                    APROF_(add_alloc)(MANGLED);
                    #endif
                    
                } else VG_(free)(mangled);
                
                DebugInfo * di = VG_(find_DebugInfo)(target);
                #if DEBUG
                APROF_(assert)(di != NULL, "Invalid debug info");
                #endif
                
                char * obj_name = (char *) VG_(DebugInfo_get_soname)(di);
                #if DEBUG
                APROF_(assert)(obj_name != NULL, "Invalid object name");
                #endif
                
                Object * obj = NULL;
                UInt hash_obj = APROF_(str_hash)(obj_name);
                obj = HT_lookup(APROF_(runtime).obj_ht, hash_obj);
                while(obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0)
                    obj = obj->next;
                
                if (obj == NULL) {
                    
                    obj_name = VG_(strdup)("copy obj", obj_name);
                    #if DEBUG
                    APROF_(assert)(obj_name != NULL, "Invalid copy of object name");
                    #endif
                    
                    obj = VG_(calloc)("obj", sizeof(Object), 1);
                    #if DEBUG
                    APROF_(assert)(obj != NULL, "Invalid object");
                    #endif
                    
                    #if DEBUG_ALLOCATION
                    APROF_(add_alloc)(OBJ_NAME);
                    APROF_(add_alloc)(OBJ);
                    #endif
                    
                    obj->name = obj_name;
                    obj->key = hash_obj;
                    
                    HT_add_node(APROF_(runtime).obj_ht, obj->key, obj);
                    
                }
                
                fn->key = target;
                fn->name = rtn_name;
                fn->obj = obj;
                
                HT_add_node(APROF_(runtime).fn_ht, fn->key, fn);
            
            }
            
            rtn_info = APROF_(new_routine_info)(tdata, fn, target);
            #if DEBUG
            APROF_(assert)(rtn_info != NULL, "Invalid routine info");
            #endif
            
        }
        
        act->rtn_info = rtn_info;
        
        APROF_(function_enter)(tdata, act);
    
    } else if (arg[2] == 2) {
        
        Activation * act = APROF_(get_activation)(tdata, tdata->stack_depth);
        #if DEBUG
        APROF_(assert)(act != NULL, "Invalid activation");
        #endif
        
        APROF_(function_exit)(tdata, act);
        
        #if DEBUG
        APROF_(assert)(tdata->stack_depth > 0, "Stack depth not possible");
        #endif
        tdata->stack_depth--;
    
    } else
        APROF_(assert)(0, "Invalid client req");
#endif    

    return True;
}
