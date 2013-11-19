/*
 * Setup of valgrind and instrumentation needed by aprof;
 * command line parser
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

Runtime APROF_(runtime);

static IRSB* APROF_(instrument) (   
                                    VgCallbackClosure * closure, 
                                    IRSB * sbIn,
                                    VexGuestLayout * layout, 
                                    VexGuestExtents * vge,
                                    VexArchInfo * archinfo_host,
                                    IRType gWordTy, 
                                    IRType hWordTy
                                ) {

    UInt  i = 0;
    IRSB * sbOut = NULL;    
    IRTypeEnv * tyenv = sbIn->tyenv;
    UInt instr_offset = 0;
    
    // Valgrind doesn't currently support this case
    APROF_(assert)(gWordTy == hWordTy, "host/guest word size mismatch");

    APROF_(runtime).events_used = 0;

    /* Set up SB */
    sbOut = deepCopyIRSBExceptStmts(sbIn);

    // Copy verbatim any IR preamble preceding the first IMark
    while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
        addStmtToIRSB( sbOut, sbIn->stmts[i] );
        i++;
    }
    
    BB * bb = NULL;
    if (APROF_(runtime).function_tracing) {
        
        bb = APROF_(get_BB)(sbIn->stmts[i]->Ist.IMark.addr +
                                + (Addr)sbIn->stmts[i]->Ist.IMark.delta);

        IRExpr  * e3 = mkIRExpr_HWord ( (HWord) bb );
        IRExpr  * e2 = mkIRExpr_HWord ( (HWord) (Addr)sbIn->stmts[i]->Ist.IMark.addr
                                        + (Addr)sbIn->stmts[i]->Ist.IMark.delta);
        IRDirty * di3 = unsafeIRDirty_0_N( 2, "BB start",
                                    VG_(fnptr_to_fnentry)( &APROF_(BB_start) ),
                                    mkIRExprVec_2( e2, e3 ) );

        addStmtToIRSB( sbOut, IRStmt_Dirty(di3) );
    }
    
    #if COST == BB_COUNT
    else {
        
        IRDirty * di2 = unsafeIRDirty_0_N(  0, "add_one_guest_BB", 
                                            VG_(fnptr_to_fnentry)( &APROF_(increase_cost) ), 
                                            mkIRExprVec_0() );
        addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
    }
    #endif
   
    for (/*use current i*/; i < sbIn->stmts_used; i++) {
        
        IRStmt * st = sbIn->stmts[i];
        if (!st || st->tag == Ist_NoOp) continue;

        switch (st->tag) {

            case Ist_NoOp:
            case Ist_AbiHint:
            case Ist_Put:
            case Ist_PutI:
            case Ist_MBE: {
                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_IMark: {
                
                #if COST == INSTR
                di = unsafeIRDirty_0_N( 0, "add_one_guest_instr",
                                        VG_(fnptr_to_fnentry)( &APROF_(increase_cost) ), 
                                        mkIRExprVec_0() );
                addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
                #endif

                APROF_(addEvent_Ir)( sbOut, mkIRExpr_HWord( (HWord)st->Ist.IMark.addr +
                                                        (Addr)st->Ist.IMark.delta),
                                        st->Ist.IMark.len );
                
                if (st->Ist.IMark.len == 0)
                    instr_offset += VG_MIN_INSTR_SZB;
                else
                    instr_offset += st->Ist.IMark.len;
                
                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_WrTmp:{

                IRExpr* data = st->Ist.WrTmp.data;
                if (data->tag == Iex_Load) {
                    
                    APROF_(addEvent_Dr)( sbOut, data->Iex.Load.addr,
                                    sizeofIRType(data->Iex.Load.ty) );
                }

                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_Store: {
                
                IRExpr * data  = st->Ist.Store.data;
                APROF_(addEvent_Dw)(sbOut, st->Ist.Store.addr,
                            sizeofIRType(typeOfIRExpr(tyenv, data)) );
                            
                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_Dirty: {

                Int dsize;
                IRDirty* d = st->Ist.Dirty.details;
                if (d->mFx != Ifx_None) {
                    
                    // This dirty helper accesses memory.  Collect the details.
                    tl_assert(d->mAddr != NULL);
                    tl_assert(d->mSize != 0);
                    dsize = d->mSize;
                    
                    if (d->mFx == Ifx_Read || d->mFx == Ifx_Modify) {
                        APROF_(addEvent_Dr)( sbOut, d->mAddr, dsize );
                    }
                    if (d->mFx == Ifx_Write || d->mFx == Ifx_Modify) {
                        APROF_(addEvent_Dw)( sbOut, d->mAddr, dsize );
                    }
                    
                }

                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_CAS: {
                
                /* We treat it as a read and a write of the location.  I
                think that is the same behaviour as it was before IRCAS
                was introduced, since prior to that point, the Vex
                front ends would translate a lock-prefixed instruction
                into a (normal) read followed by a (normal) write. */
                Int    dataSize;
                IRType dataTy;
                IRCAS* cas = st->Ist.CAS.details;
                tl_assert(cas->addr != NULL);
                tl_assert(cas->dataLo != NULL);
                dataTy   = typeOfIRExpr(tyenv, cas->dataLo);
                dataSize = sizeofIRType(dataTy);
                if (cas->dataHi != NULL)
                    dataSize *= 2; /* since it's a doubleword-CAS */
                
                APROF_(addEvent_Dr)( sbOut, cas->addr, dataSize );
                APROF_(addEvent_Dw)( sbOut, cas->addr, dataSize );

                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_LLSC: {
                
                IRType dataTy;
                if (st->Ist.LLSC.storedata == NULL) {
                    /* LL */
                    dataTy = typeOfIRTemp(tyenv, st->Ist.LLSC.result);
                    APROF_(addEvent_Dr)( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
                } else {
                    /* SC */
                    dataTy = typeOfIRExpr(tyenv, st->Ist.LLSC.storedata);
                    APROF_(addEvent_Dw)( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
                }
                
                addStmtToIRSB( sbOut, st );
                break;
            }

            case Ist_Exit: {
                
                APROF_(runtime).flush_events(sbOut);
                
                addStmtToIRSB( sbOut, st );      // Original statement
                break;
            }

            default:
                tl_assert(0);
        }
    }

    if (APROF_(runtime).function_tracing) {
        
        if (sbIn->jumpkind == Ijk_Call) bb->exit = BBCALL;
        else if (sbIn->jumpkind == Ijk_Ret) bb->exit = BBRET;
        else bb->exit = BBOTHER;
        
        /* 
         * Set a global var last_exit (saved/restored when there is
         * a switch btw thread) to the type of final jump for this BB
         */
        addStmtToIRSB( sbOut, IRStmt_Store(Endness, 
                        IRExpr_Const(
                                        hWordTy == Ity_I32 ?
                                        IRConst_U32( (UWord) &(APROF_(runtime).last_exit) ) :
                                        IRConst_U64( (UWord) &(APROF_(runtime).last_exit) )
                                    ),
                        IRExpr_Const(IRConst_U32(bb->exit))
                        )
                    );
        
        bb->instr_offset = instr_offset;
    }
    
    /* At the end of the sbIn.  Flush outstandings. */
    APROF_(runtime).flush_events(sbOut);

    return sbOut;
}

static void APROF_(default_params)(void) {
    
    // default values
    APROF_(runtime).memory_resolution = 4;
    APROF_(runtime).log_dir = NULL;
    APROF_(runtime).collect_CCT = False;
    APROF_(runtime).function_tracing = True;
    APROF_(runtime).input_metric = RMS;
    APROF_(runtime).incremental_report = True;
    APROF_(runtime).single_report = True;
}

// aprof initialization
// some runtime parameters are set by APROF_(cmd_line)()
static void APROF_(init)(void) {
    
    if (APROF_(runtime).function_tracing)
        APROF_(runtime).bb_ht = HT_construct(VG_(free));
    else
        APROF_(runtime).bb_ht = NULL;
    
    APROF_(assert)(APROF_(runtime).input_metric == RMS 
                    || APROF_(runtime).input_metric == DRMS,
                    "Invalid input size metric");
    
    if (APROF_(runtime).input_metric == DRMS) {
        
        APROF_(runtime).global_shadow_memory = LK_create();
        APROF_(runtime).flush_events = APROF_(flush_events_drms);
        
        VG_(needs_syscall_wrapper) (
                                        APROF_(pre_syscall), 
                                        APROF_(post_syscall)
                                    );
    
    } else {
        APROF_(runtime).global_shadow_memory = NULL;
        APROF_(runtime).flush_events = APROF_(flush_events_rms);
    }
    
    APROF_(runtime).fn_ht = HT_construct(NULL);    
    APROF_(runtime).obj_ht = HT_construct(NULL);
    APROF_(runtime).global_counter = 1;
    APROF_(runtime).events_used = 0;
    APROF_(runtime).current_TID = VG_INVALID_THREADID;
    APROF_(runtime).current_tdata = NULL;
    APROF_(runtime).running_threads = 0;
    APROF_(runtime).last_bb = NULL;
    APROF_(runtime).last_exit = NONE;
    APROF_(runtime).dl_runtime_resolve_addr = 0;
    APROF_(runtime).dl_runtime_resolve_length = 0;
    APROF_(runtime).next_function_id = 0;
    
    if (APROF_(runtime).collect_CCT && APROF_(runtime).single_report)
        APROF_(runtime).root = APROF_(new)(CCT_S, sizeof(CCTNode));
        
    APROF_(runtime).application = VG_(strdup)("app", VG_(args_the_exename));
    APROF_(runtime).cmd_line = VG_(malloc)("cmd", 4096);
    
    // write commandline
    XArray * x = VG_(args_for_client);
    UInt i = 0, offset = 0;
    offset += VG_(sprintf)(APROF_(runtime).cmd_line, "%s", APROF_(runtime).application);
    for (i = 0; i < VG_(sizeXA)(x); i++) {
        HChar ** c = VG_(indexXA)(x, i);
        if (c != NULL) {
            offset += VG_(sprintf)(APROF_(runtime).cmd_line, " %s", *c);
        }
        APROF_(assert)(offset < 4096, "command line too long\n");
    }
    
    VG_(clo_vex_control).iropt_unroll_thresh = 0;
    VG_(clo_vex_control).guest_chase_thresh  = 0;
}

#if MEM_USAGE_INFO 
void APROF_(print_info_mem_usage)(void) {
    
    //VG_(umsg)("Getting info about memory\n");
    
    HChar buf[NAME_SIZE];
    VG_(sprintf)(buf, "/proc/%d/status", VG_(gettid)());
    SysRes res = VG_(open)(buf, VKI_O_RDONLY, 0);
    Int file = (Int) sr_Res(res);
    if (file < 0) {
        VG_(umsg)("No info about memory usage [1]\n");
        return;
    }
    
    UInt rb = 0;
    Int ret;
    while (rb < NAME_SIZE) {
        ret = VG_(read)(file, buf + rb, NAME_SIZE - rb);
        if (ret > 0) rb = ret;
        else break;
    }
    
    VG_(close)(file);
    
    ret = 0;
    const HChar * sentinel = "VmPeak:	";
    UInt sent_pos = 0;
    UInt sent_len = VG_(strlen)(sentinel);
    while (rb > 0) {

        //VG_(printf)("%c", buf[ret]);
        rb--; 
        
        if (buf[ret++] == sentinel[sent_pos]) {
            
            sent_pos++;
            if (sent_pos == sent_len) break;
                
        } else if (sent_pos > 0)
            sent_pos = 0;

    }
    
    if (sent_pos == sent_len) {
        VG_(printf)("==%d== Memory used by aprof:", VG_(getpid)());
        while (rb > 0) {
            
            VG_(printf)("%c", buf[ret]);
            if (buf[ret++] == '\n') break;
            rb--;
        
        }
    } else
        VG_(umsg)("No info about memory usage [2]\n");
}

#endif

/* aprof finalization */
static void APROF_(fini)(Int exitcode) {
    
    APROF_(kill_threads)();
    
    if (APROF_(runtime).function_tracing) {
    
        #if DEBUG_ALLOCATION
        UInt j = 0;
        for (j = 0; j < HT_count_nodes(APROF_(runtime).bb_ht); j++)
            APROF_(remove_alloc)(BB_S);
        #endif
        
        HT_destruct(APROF_(runtime).bb_ht);
    }
    
    // destroy function objs
    HT_ResetIter(APROF_(runtime).fn_ht);
    Function * f = HT_RemoveNext(APROF_(runtime).fn_ht);
    while (f != NULL) {
        
        APROF_(delete)(FN_NAME_S, f->name);
        if (f->mangled) APROF_(delete)(MANGLED_S, f->mangled);
        APROF_(delete)(FN_S, f);
        
        f = HT_RemoveNext(APROF_(runtime).fn_ht);
    }
    HT_destruct(APROF_(runtime).fn_ht);
    
    // destroy ELF objects
    HT_ResetIter(APROF_(runtime).obj_ht);
    Object * o = HT_RemoveNext(APROF_(runtime).obj_ht);
    while (o != NULL) {
        
        APROF_(delete)(OBJ_NAME_S, o->name);
        APROF_(delete)(OBJ_S, o);
        o = HT_RemoveNext(APROF_(runtime).obj_ht);
    }
    HT_destruct(APROF_(runtime).obj_ht);
    
    if (APROF_(runtime).input_metric == DRMS)
        LK_destroy(APROF_(runtime).global_shadow_memory);
    
    APROF_(print_alloc)();
}

#if 0
/*
 * I don't have yet tested what happens when a singnal
 * is received wrt shadow stack 
 */
static void APROF_(signal)(ThreadId tid, Int sigNo, Bool alt_stack) {
    AP_ASSERT(0, "There is a signal");
}
#endif

static Bool APROF_(cmd_line)(const HChar* argv) {
    
    Bool value_bool = False;
    int value_int = 0;
    
    if VG_INT_CLO(argv, "--memory-resolution", value_int) {
        
        APROF_(assert)(value_int == 1 
                        || value_int == 2
                        || value_int == 4
                        || value_int == 8
                        || value_int == 16,
                        "Invalid memory resolution");
        
        APROF_(runtime).memory_resolution = value_int;
    }

    else if VG_STR_CLO(argv, "--log-dir", APROF_(runtime).log_dir) {}
    
    else if VG_BOOL_CLO(argv, "--collect-cct", APROF_(runtime).collect_CCT) {}
    
    else if VG_BOOL_CLO(argv, "--internal-fn-tracing", APROF_(runtime).function_tracing) {}
    
    else if VG_BOOL_CLO(argv, "--drms", value_bool) {
        if (value_bool) APROF_(runtime).input_metric = DRMS;
    }
    
    return True;
}

static void APROF_(print_usage)(void) {
    
    VG_(printf)(
        "    --memory-resolution=<n>        Memory resolution of the shadow memory {1, 2, 4, 8, 16} [4]\n"
        "    --log-dir=<PATH>               Reports will be saved in this directory\n"
        "    --drms=no|yes                  Use the dynamic read memory size [no]\n"
        "    --collect-cct=no|yes           Collect calling contect tree [no]\n"
    //    "    --internal-fn-tracing=yes|no   Use the internal function CALL/RET heuristic [yes]\n"
    );
}

/* Valgrind init */
static void APROF_(pre_clo_init)(void) {

    VG_(details_name)             ("aprof");
    VG_(details_version)          (APROF_VERSION);
    VG_(details_description)      ("Input-sensitive Profiler - http://code.google.com/p/aprof/");
    VG_(details_copyright_author) ("by Emilio Coppa, Camil Demetrescu, Irene Finocchi, Romolo Marotta");
    VG_(details_bug_reports_to)   ("ercoppa@gmail.com");

    VG_(basic_tool_funcs)           (    
                                        APROF_(init), 
                                        APROF_(instrument), 
                                        APROF_(fini)
                                    );
    
    VG_(needs_command_line_options) (
                                        APROF_(cmd_line), 
                                        APROF_(print_usage),
                                        NULL
                                    );
    
    #if 0
    VG_(needs_client_requests)      (APROF_(trace_function));
    #endif
    
    VG_(track_pre_thread_ll_create) (APROF_(thread_create));
    VG_(track_start_client_code)    (APROF_(thread_switch));
    VG_(track_pre_thread_ll_exit)   (APROF_(thread_exit));
    //VG_(track_pre_deliver_signal)   (APROF_(signal));
    
    VG_(clo_vex_control).iropt_unroll_thresh = 0;
    VG_(clo_vex_control).guest_chase_thresh  = 0;
    APROF_(default_params)();
}

VG_DETERMINE_INTERFACE_VERSION(APROF_(pre_clo_init))
