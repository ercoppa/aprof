/*
 * Setup of valgrind and instrumentation  
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

typedef struct act {
	UWord sp;
	UWord target;
	Char * fn;
} act;

static UWord stack_depth = 0;
static act * current = NULL; 
static act * tstack = NULL;

typedef enum jump_t { BB_INIT, CALL, RET, BBCALL, BBRET, BOR, BBBOR } jump_t;

static VG_REGPARM(2) void call(UWord target, UWord type_op) {

	UWord type = 0;
	if (type_op == BB_INIT) type = 1;
	else if (type_op == CALL) type = 2;
	else if (type_op == RET) type = 3;
	else if (type_op == BBCALL) type = 4;
	else if (type_op == BBRET) type = 5;
	else if (type_op == BOR) type = 6;
	else if (type_op == BBBOR) type = 7;
	else failure("Invalid type");
	
	UWord csp = (UWord) VG_(get_SP)(1);
	Char * fn = VG_(calloc)("fn name", 256, 1);
	
	/* Init stack */
	if (tstack == NULL) {
		
		tstack = VG_(calloc)("thread_data", sizeof(act) * 4096, 1);
		current = tstack;
		VG_(sprintf)(fn, "(below main)");
		current->fn = fn;
		current->sp = csp;
		
		if (VG_(get_fnname_if_entry)(target, fn, 256)) failure("Wrong");
		
		return;
	
	}
	
	/*
	if ((type == 6 || type == 7) && csp != current->sp) {
		VG_(printf)("BORING: SP: %lu ~ CSP: %lu\n", csp, current->sp);
	}*/

	if (VG_(get_fnname_if_entry)(target, fn, 256)) {
		
		stack_depth++;
		current++;
		current->sp = csp;
		current->target = target;
		current->fn = fn;
		VG_(printf)("[%lu] Entered: %s from %s ~ SP: %lu ~ T: %lu ~ Type %lu\n",
							stack_depth, fn, (current-1)->fn, csp, target, type);
		
	} else if (csp > current->sp) {
		
		if (!VG_(get_fnname)(target, fn, 256)) {
		
			/*
		
			VG_(get_fnname_w_offset) (target, fn, 256);
			VG_(printf)("Function: %s\n", fn);
			
			failure("Invalid function name");
			*/

		}
		
		while(stack_depth > 0) {
			
			VG_(printf)("[%lu] Return from %s to %s ~ CSP: %lu ~ SP: %lu ~  OLDSP: %lu ~ T: %lu ~ Type %lu\n",
						stack_depth, current->fn, (current-1)->fn, csp, current->sp, (current+1)->sp, target, type);
			
			stack_depth--;
			current--;
			
			if (csp <= current->sp) {
				
				VG_(printf)("[%lu] Inside %s ~ CSP: %lu ~ SP: %lu ~  OLDSP: %lu ~ T: %lu ~ Type %lu\n",
					stack_depth, current->fn, csp, current->sp, (current+1)->sp, target, type);
				
				if (VG_(strcmp)(current->fn,fn) != 0 && VG_(strcmp)("",fn) != 0) {
					VG_(printf)("Simulated stack says you are in %s but valgrind says %s\n", current->fn, fn);
					failure("Mismatch during return\n");
				}
				break;
			}
		}
		
		if (stack_depth == 0 && current->sp < csp) {
			
			if (current->sp == 0)
				VG_(printf)("Init start SP\n");
			
			else
				VG_(printf)("Redefining start SP\n");
				
			current->sp = csp;
			
		}
		/*
		if (stack_depth > 0)
			VG_(printf)("[%lu] Return from %s to %s ~ CSP: %lu ~ SP: %lu ~  OLDSP: %lu ~ T: %lu ~ Type %lu\n",
						stack_depth, current->fn, (current-1)->fn, csp, current->sp, (current+1)->sp, target, type);
		
		else
			VG_(printf)("[%lu] Return from %s ~ CSP: %lu ~ SP: %lu ~  OLDSP: %lu ~ T: %lu ~ Type %lu\n",
						stack_depth, current->fn, csp, current->sp, (current+1)->sp, target, type);
		*/
	} else if (csp < current->sp) {
		
		if (VG_(get_fnname)(target, fn, 256)) {
			if (VG_(strcmp)(current->fn,fn) != 0 && VG_(strcmp)(fn, "") != 0) {
				
				stack_depth++;
				current++;
				current->sp = csp;
				current->target = target;
				current->fn = fn;
				VG_(printf)("[%lu] Entered: %s from %s ~ SP: %lu ~ T: %lu ~ Type %lu\n",
				stack_depth, fn, (current-1)->fn, csp, target, type);
				

				VG_(printf)("Simulated stack says you are in %s but valgrind says %s\n", current->fn, fn);
				failure("Mismatch during call\n");

			}
		}
		
	}

}

static
IRSB* instrument (  VgCallbackClosure* closure, 
					IRSB* sbIn,
					VexGuestLayout* layout, 
					VexGuestExtents* vge,
					IRType gWordTy, IRType hWordTy ) {

	Int        i;
	IRSB*      sbOut;
	IRTypeEnv* tyenv = sbIn->tyenv;

	if (gWordTy != hWordTy) /* We don't currently support this case. */
		VG_(tool_panic)("host/guest word size mismatch");

	/* Set up SB */
	sbOut = deepCopyIRSBExceptStmts(sbIn);

	// Copy verbatim any IR preamble preceding the first IMark
	i = 0;
	while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
		addStmtToIRSB( sbOut, sbIn->stmts[i] );
		i++;
	}
	
	#if EVENTCOUNT == 0 || EVENTCOUNT >= 2
	
	#if TIME == BB_COUNT
	IRDirty * di2 = unsafeIRDirty_0_N( 0, "add_one_guest_BB", 
	VG_(fnptr_to_fnentry)( &add_one_guest_BB ), 
								mkIRExprVec_0() );
	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	#endif
	
	#endif
	
	if (sbIn->stmts[i]->tag != Ist_IMark) failure("errore");
	
	IRExpr  * e1 = mkIRExpr_HWord ( BB_INIT );
	IRExpr  * e2 = mkIRExpr_HWord ( (HWord) sbIn->stmts[i]->Ist.IMark.addr );
	IRDirty * di2 = unsafeIRDirty_0_N( 2, "call",
								VG_(fnptr_to_fnentry)( &call ),
								mkIRExprVec_2( e2, e1 ) );

	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	
	for (/*use current i*/; i < sbIn->stmts_used; i++) {
		
		IRStmt* st = sbIn->stmts[i];
		if (!st || st->tag == Ist_NoOp) continue;

		/* Count one VEX statement.
		#if COUNT_VEX_I
		di = unsafeIRDirty_0_N( 0, "add_one_IRStmt", 
		VG_(fnptr_to_fnentry)( &add_one_IRStmt ), 
		mkIRExprVec_0() );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		#endif
		*/

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

				#if EVENTCOUNT == 0 || EVENTCOUNT >= 2

				#if TIME == INSTR
				di = unsafeIRDirty_0_N( 0, "add_one_guest_instr",
										VG_(fnptr_to_fnentry)( &add_one_guest_instr ), 
										mkIRExprVec_0() );
				addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
				#endif
				
				#endif

				#if EVENTCOUNT != 2
				addEvent_Ir( sbOut, mkIRExpr_HWord( (HWord)st->Ist.IMark.addr ),
								st->Ist.IMark.len );
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_WrTmp:{

				#if EVENTCOUNT != 2 
				IRExpr* data = st->Ist.WrTmp.data;
				if (data->tag == Iex_Load) {
					addEvent_Dr( sbOut, data->Iex.Load.addr,
					sizeofIRType(data->Iex.Load.ty) );
				}
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Store: {
				
				#if EVENTCOUNT != 2
				IRExpr* data  = st->Ist.Store.data;
				addEvent_Dw(sbOut, st->Ist.Store.addr,
							sizeofIRType(typeOfIRExpr(tyenv, data)) );
				#endif
									
				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Dirty: {

				#if EVENTCOUNT != 2
				Int dsize;
				IRDirty* d = st->Ist.Dirty.details;
				if (d->mFx != Ifx_None) {
					// This dirty helper accesses memory.  Collect the details.
					tl_assert(d->mAddr != NULL);
					tl_assert(d->mSize != 0);
					dsize = d->mSize;
					if (d->mFx == Ifx_Read || d->mFx == Ifx_Modify)
						addEvent_Dr( sbOut, d->mAddr, dsize );
					if (d->mFx == Ifx_Write || d->mFx == Ifx_Modify)
						addEvent_Dw( sbOut, d->mAddr, dsize );
				} else {
					tl_assert(d->mAddr == NULL);
					tl_assert(d->mSize == 0);
				}
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_CAS: {
				
				#if EVENTCOUNT != 2
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

				addEvent_Dr( sbOut, cas->addr, dataSize );
				addEvent_Dw( sbOut, cas->addr, dataSize );
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_LLSC: {
				
				#if EVENTCOUNT != 2
				IRType dataTy;
				if (st->Ist.LLSC.storedata == NULL) {
					/* LL */
					dataTy = typeOfIRTemp(tyenv, st->Ist.LLSC.result);
					addEvent_Dr( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
				} else {
					/* SC */
					dataTy = typeOfIRExpr(tyenv, st->Ist.LLSC.storedata);
					addEvent_Dw( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
				}
				#endif
				
				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Exit: {
				
				/*
				if (st->Ist.Exit.jk == Ijk_Call) {
					
					e1 = mkIRExpr_HWord ( CALL );
					IRConst * c = st->Ist.Exit.dst;
					IRExpr  * e = IRExpr_Const( c );
					IRDirty * di = unsafeIRDirty_0_N( 2, "call",
											VG_(fnptr_to_fnentry)( &call ),
											mkIRExprVec_2( e, e1 ) );
					addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
				}
				
				if (st->Ist.Exit.jk == Ijk_Ret) {
					
					IRConst * c = st->Ist.Exit.dst;
					IRExpr  * e = IRExpr_Const( c );
					e1 = mkIRExpr_HWord ( RET );
					IRDirty * di = unsafeIRDirty_0_N( 2, "call",
											VG_(fnptr_to_fnentry)( &call ),
											mkIRExprVec_2( e, e1 ) );
					addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
				}
				
			
				if (st->Ist.Exit.jk == Ijk_Boring) {
					
					IRConst * c = st->Ist.Exit.dst;
					IRExpr  * e = IRExpr_Const( c );
					e1 = mkIRExpr_HWord ( BOR );
					IRDirty * di = unsafeIRDirty_0_N( 2, "call",
											VG_(fnptr_to_fnentry)( &call ),
											mkIRExprVec_2( e, e1 ) );
					addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
				}
				*/
				
				addStmtToIRSB( sbOut, st );      // Original statement
				break;
			}

			default:
				tl_assert(0);
		}
	}

	/* At the end of the sbIn.  Flush outstandings. */
	flushEvents(sbOut);

	/*
	if (sbIn->jumpkind == Ijk_Call) {
		
		e1 = mkIRExpr_HWord ( BBCALL );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call",
								VG_(fnptr_to_fnentry)( &call ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else if (sbIn->jumpkind == Ijk_Call) {
		
		e1 = mkIRExpr_HWord ( BBRET );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call",
								VG_(fnptr_to_fnentry)( &call ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else if (sbIn->jumpkind == Ijk_Boring) {
		
		e1 = mkIRExpr_HWord ( BBBOR );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call",
								VG_(fnptr_to_fnentry)( &call ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	}
	*/

	return sbOut;
}

/* aprof initialization */
static void post_clo_init(void) {
	return;
}

/* Funzione per presentare risultati in fase finale */
static void fini(Int exitcode) {
	
	VG_(printf)("Fine\n");
	
	#if !EVENTCOUNT && !TRACER
	HT_destroy_pool();
	#endif
	
	#if SUF == 2 && SUF2_SEARCH == STATS
	VG_(printf)("Average stack depth: %llu / %llu = %llu\n", avg_depth, ops, avg_depth/ops);
	VG_(printf)("Average # iterations: %llu / %llu = %llu\n", avg_iteration, ops, avg_iteration/ops);
	#endif
	
	#if EVENTCOUNT && !TRACER
	VG_(printf)("Load: %lu\nStore: %lu\nModify: %lu\n", read_n, write_n, modify_n);
	VG_(printf)("Function entry: %lu\nFunction exit: %lu\n", fn_in_n, fn_out_n);
	VG_(printf)("Thread: %lu\n", thread_n);
	VG_(printf)("BB executed: %lu\n", bb_c);
	#endif

}

/* Valgrind init */
static void pre_clo_init(void) {

	VG_(details_name)				("Aprof");
	VG_(details_version)			("0.1_alpha");
	VG_(details_description)		("Asymptotic Profiler");
	VG_(details_copyright_author)	("By Camil Demetrescu, Irene Finocchi, Bruno Aleandri, Emilio Coppa");
	VG_(details_bug_reports_to)		("ercoppa@gmail.com");

	VG_(basic_tool_funcs) 				(post_clo_init, instrument, fini);
	VG_(needs_client_requests)			(trace_function);
	VG_(track_pre_thread_first_insn)	(thread_start);
	VG_(track_pre_thread_ll_exit)		(thread_exit);
	
	//#if TIME == BB_COUNT
	VG_(clo_vex_control).iropt_level = 0;
	VG_(clo_vex_control).iropt_unroll_thresh = 0;
	VG_(clo_vex_control).guest_chase_thresh  = 0;
	//#endif
	
}

VG_DETERMINE_INTERFACE_VERSION(pre_clo_init)
