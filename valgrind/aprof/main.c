/*
 * Setup of valgrind and instrumentation  
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

#if MEM_TRACE && IGNORE_REPEAT_ACC

#if !COSTANT_MEM_ACCESS
#error "IGNORE_REPEAT_ACC only works with COSTANT_MEM_ACCESS enabled"
#endif

#define INIT_SIZE_ADDR_ACC 32
static UInt addr_accessed_size = INIT_SIZE_ADDR_ACC;
static IRExpr ** addr_accessed = NULL;

static Bool do_access(IRExpr * e) {
	
	#if DEBUG
	AP_ASSERT(e != NULL, "Invalid IRExpr");
	AP_ASSERT(isIRAtom(e), "Invalid IRExpr");
	#endif
	
	int i = 0;
	for(; i < addr_accessed_size; i++) {
		if (addr_accessed[i] == NULL) break;
		if (eqIRAtom(addr_accessed[i], e)) {
			//VG_(printf)("Addr already accessed\n");
			//addr_accessed[i] = e;
			return False;
		}
	}
	
	if (i >= addr_accessed_size) {
		//VG_(printf)("Relocate addr_accessed\n");
		IRExpr ** addr_accessed2 = VG_(calloc)("addr acc", 
								sizeof(IRExpr *), addr_accessed_size*2);
		int j = 0;
		for (; j < addr_accessed_size; j++) 
			addr_accessed2[j] = addr_accessed[j];
		addr_accessed_size = addr_accessed_size * 2;
		VG_(free)(addr_accessed);
		addr_accessed = addr_accessed2;
	} 
	addr_accessed[i] = e;
	
	return True;
}

#endif

extern Int events_used;

static
IRSB* instrument (  VgCallbackClosure* closure, 
					IRSB* sbIn,
					VexGuestLayout* layout, 
					VexGuestExtents* vge,
					IRType gWordTy, IRType hWordTy ) {

	Int        i;
	IRSB*      sbOut;
	IRTypeEnv* tyenv = sbIn->tyenv;
	events_used = 0;

	#if MEM_TRACE && IGNORE_REPEAT_ACC
	addr_accessed = VG_(calloc)("addr acc", sizeof(IRExpr *), addr_accessed_size);
	#endif
	
	#if TRACE_FUNCTION
	UInt instr_offset = 0;
	#endif

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
	
	#if TRACE_FUNCTION
	
	BB * bb = get_BB(sbIn->stmts[i]->Ist.IMark.addr);
	#if DEBUG
	AP_ASSERT(bb != NULL, "Invalid BB")
	#endif

	IRExpr  * e3 = mkIRExpr_HWord ( (HWord) bb );
	IRExpr  * e2 = mkIRExpr_HWord ( (HWord) (Addr)sbIn->stmts[i]->Ist.IMark.addr );
	IRDirty * di3 = unsafeIRDirty_0_N( 2, "BB start",
								VG_(fnptr_to_fnentry)( &BB_start ),
								mkIRExprVec_2( e2, e3 ) );

	addStmtToIRSB( sbOut, IRStmt_Dirty(di3) );
	
	#endif
	
	#if TIME == BB_COUNT && !TRACE_FUNCTION
	IRDirty * di2 = unsafeIRDirty_0_N( 0, "add_one_guest_BB", 
	VG_(fnptr_to_fnentry)( &add_one_guest_BB ), 
								mkIRExprVec_0() );
	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	#endif
   
	#if MEM_TRACE && IGNORE_REPEAT_ACC
	Char * helperNameA;
	void * helperAddrA;
	IRExpr * * argvA;
	IRDirty * diA;
	#endif
   
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

				#if TIME == INSTR
				di = unsafeIRDirty_0_N( 0, "add_one_guest_instr",
										VG_(fnptr_to_fnentry)( &add_one_guest_instr ), 
										mkIRExprVec_0() );
				addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
				#endif

				#if MEM_TRACE
				
				#if IGNORE_REPEAT_ACC
				//do_access(mkIRExpr_HWord( (HWord)st->Ist.IMark.addr ));
				#else
				addEvent_Ir( sbOut, mkIRExpr_HWord( (HWord)st->Ist.IMark.addr ),
								st->Ist.IMark.len );
				#endif
				
				#endif
				
				#if TRACE_FUNCTION
				if (st->Ist.IMark.len == 0)
					instr_offset += VG_MIN_INSTR_SZB;
				else
					instr_offset += st->Ist.IMark.len;
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_WrTmp:{

				#if MEM_TRACE
				IRExpr* data = st->Ist.WrTmp.data;
				if (data->tag == Iex_Load) {
					#if IGNORE_REPEAT_ACC
					IRExpr * a = data->Iex.Load.addr;
					if (do_access(a)) {
						helperNameA = "trace_load";
						argvA = mkIRExprVec_3(	mkIRExpr_HWord(LOAD),
												a, 
												mkIRExpr_HWord( sizeofIRType(data->Iex.Load.ty) )
											);
						helperAddrA = trace_access;
						diA = unsafeIRDirty_0_N( 3, helperNameA, 
									VG_(fnptr_to_fnentry)( helperAddrA ),
									argvA );
						addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
					}
					#else
					addEvent_Dr( sbOut, data->Iex.Load.addr,
									sizeofIRType(data->Iex.Load.ty) );
					#endif
				}
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Store: {
				
				#if MEM_TRACE
				IRExpr * data  = st->Ist.Store.data;
				#if IGNORE_REPEAT_ACC
				IRExpr * a = st->Ist.Store.addr;
				if (do_access(a)) {
					helperNameA = "trace_store";
					argvA = mkIRExprVec_3(	mkIRExpr_HWord(STORE),
											a, 
											mkIRExpr_HWord( sizeofIRType(typeOfIRExpr(tyenv, data)) )
										);
					helperAddrA = trace_access;
					diA = unsafeIRDirty_0_N( 3, helperNameA, 
								VG_(fnptr_to_fnentry)( helperAddrA ),
								argvA );
					addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
				}
				#else
				addEvent_Dw(sbOut, st->Ist.Store.addr,
							sizeofIRType(typeOfIRExpr(tyenv, data)) );
				#endif
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Dirty: {

				#if MEM_TRACE
				Int dsize;
				IRDirty* d = st->Ist.Dirty.details;
				if (d->mFx != Ifx_None) {
					// This dirty helper accesses memory.  Collect the details.
					tl_assert(d->mAddr != NULL);
					tl_assert(d->mSize != 0);
					dsize = d->mSize;
					#if IGNORE_REPEAT_ACC
					IRExpr * a = d->mAddr;
					if (d->mFx == Ifx_Read || d->mFx == Ifx_Modify) {
						if (do_access(a)) {
							helperNameA = "trace_load";
							argvA = mkIRExprVec_3(	mkIRExpr_HWord(LOAD),
														a, 
														mkIRExpr_HWord( dsize )
													);
							helperAddrA = trace_access;
							diA = unsafeIRDirty_0_N( 3, helperNameA, 
										VG_(fnptr_to_fnentry)( helperAddrA ),
										argvA );
							addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
						}
					}
					else if (d->mFx == Ifx_Write) {
						if (do_access(a)) {
							helperNameA = "trace_store";
							argvA = mkIRExprVec_3(	mkIRExpr_HWord(STORE),
														a, 
														mkIRExpr_HWord( dsize )
													);
							helperAddrA = trace_access;
							diA = unsafeIRDirty_0_N( 3, helperNameA, 
										VG_(fnptr_to_fnentry)( helperAddrA ),
										argvA );
							addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
						}
					}
					#else
					if (d->mFx == Ifx_Read || d->mFx == Ifx_Modify) {
						addEvent_Dr( sbOut, d->mAddr, dsize );
					}
					if (d->mFx == Ifx_Write || d->mFx == Ifx_Modify) {
						addEvent_Dw( sbOut, d->mAddr, dsize );
					}
					#endif
				} else {
					tl_assert(d->mAddr == NULL);
					tl_assert(d->mSize == 0);
				}
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_CAS: {
				
				#if MEM_TRACE
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
				
				#if IGNORE_REPEAT_ACC
				IRExpr * a = cas->addr;
				if (do_access(a)) {
					helperNameA = "trace_load";
					argvA = mkIRExprVec_3(	mkIRExpr_HWord(LOAD),
											a, 
											mkIRExpr_HWord( dataSize )
										);
					helperAddrA = trace_access;
					diA = unsafeIRDirty_0_N( 3, helperNameA, 
								VG_(fnptr_to_fnentry)( helperAddrA ),
								argvA );
					addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
				}
				#else
				addEvent_Dr( sbOut, cas->addr, dataSize );
				addEvent_Dw( sbOut, cas->addr, dataSize );
				#endif
				
				#endif

				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_LLSC: {
				
				#if MEM_TRACE
				IRType dataTy;
				if (st->Ist.LLSC.storedata == NULL) {
					/* LL */
					dataTy = typeOfIRTemp(tyenv, st->Ist.LLSC.result);
					#if IGNORE_REPEAT_ACC
					IRExpr * a = st->Ist.LLSC.addr;
					if (do_access(a)) {
						helperNameA = "trace_load";
						argvA = mkIRExprVec_3(	mkIRExpr_HWord(LOAD),
												a, 
												mkIRExpr_HWord( sizeofIRType(dataTy) )
											);
						helperAddrA = trace_access;
						diA = unsafeIRDirty_0_N( 3, helperNameA, 
									VG_(fnptr_to_fnentry)( helperAddrA ),
									argvA );
						addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
					}
					#else
					addEvent_Dr( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
					#endif
				} else {
					/* SC */
					dataTy = typeOfIRExpr(tyenv, st->Ist.LLSC.storedata);
					#if IGNORE_REPEAT_ACC
					IRExpr * a = st->Ist.LLSC.addr;
					if (do_access(a)) {
						helperNameA = "trace_store";
						argvA = mkIRExprVec_3(	mkIRExpr_HWord(STORE),
												a, 
												mkIRExpr_HWord( sizeofIRType(dataTy) )
											);
						helperAddrA = trace_access;
						diA = unsafeIRDirty_0_N( 3, helperNameA, 
									VG_(fnptr_to_fnentry)( helperAddrA ),
									argvA );
						addStmtToIRSB( sbOut, IRStmt_Dirty(diA) );
					}
					#else
					addEvent_Dw( sbOut, st->Ist.LLSC.addr, sizeofIRType(dataTy) );
					#endif
				}
				#endif
				
				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_Exit: {
				#if MEM_TRACE && !IGNORE_REPEAT_ACC
				flushEvents(sbOut);
				#endif
				addStmtToIRSB( sbOut, st );      // Original statement
				break;
			}

			default:
				tl_assert(0);
		}
	}

	#if TRACE_FUNCTION
	
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
									IRConst_U32( (UWord) &last_exit ) :
									IRConst_U64( (UWord) &last_exit )
								),
					IRExpr_Const(IRConst_U32(bb->exit))
					)
				);
	
	bb->instr_offset = instr_offset;
	
	#endif

	#if MEM_TRACE
	
	#if !IGNORE_REPEAT_ACC
	/* At the end of the sbIn.  Flush outstandings. */
	flushEvents(sbOut);
	#else
	VG_(free)(addr_accessed);
	addr_accessed_size = INIT_SIZE_ADDR_ACC;
	#endif
	
	#endif

	return sbOut;
}

/* aprof initialization */
static void post_clo_init(void) {
	
	#if TRACE_FUNCTION
	bb_ht = HT_construct(VG_(free));
	AP_ASSERT(bb_ht != NULL, "bb ht not allocable");
	
	#if DEBUG_ALLOCATION
	add_alloc(HT);
	#endif
	
	#endif
	
	fn_ht = HT_construct(VG_(free));
	AP_ASSERT(fn_ht != NULL, "fn ht not allocable");
	
	obj_ht = HT_construct(VG_(free));
	AP_ASSERT(obj_ht != NULL, "fn ht not allocable");
	
	#if DEBUG_ALLOCATION
	add_alloc(HT);
	#endif
	
}

/* Funzione per presentare risultati in fase finale */
static void fini(Int exitcode) {
	
	#if DEBUG_ALLOCATION
	print_alloc();
	#endif
	
	#if TRACE_FUNCTION 
	HT_destruct(bb_ht);
	#endif
	
	HT_destruct(fn_ht);
	HT_destruct(obj_ht);

	#if 0
	/* test compression SUF2 */
	StackUF * uf = SUF_create();
	SUF_insert(uf, 0, 5);
	SUF_insert(uf, 5, 5);
	SUF_insert(uf, 9, 5);
	SUF_print(uf);
	SUF_insert(uf, 13, 10);
	SUF_insert(uf, 5, 10);
	SUF_insert(uf, 9, 10);
	SUF_print(uf);
	SUF_insert(uf, 15, 15);
	SUF_insert(uf, 9, 15);
	SUF_print(uf);
	SUF_insert(uf, 15, 20);
	SUF_print(uf);
	
	UInt arr_aid[2] = { 5, 20 };
	SUF_compress(uf, arr_aid, 2);
	SUF_print(uf);
	#endif

}

void signal(ThreadId tid, Int sigNo, Bool alt_stack);
void signal(ThreadId tid, Int sigNo, Bool alt_stack) {
	AP_ASSERT(0, "There is a signal");
}


/* Valgrind init */
static void pre_clo_init(void) {

	VG_(details_name)				("aprof");
	VG_(details_version)			("0.1");
	VG_(details_description)		("Input-sensitive Profiler - http://code.google.com/p/aprof/");
	VG_(details_copyright_author)	("By Emilio Coppa, Camil Demetrescu, Irene Finocchi");
	VG_(details_bug_reports_to)		("ercoppa@gmail.com");

	VG_(basic_tool_funcs) 			(post_clo_init, instrument, fini);
	
	#if !TRACE_FUNCTION
	VG_(needs_client_requests)		(trace_function);
	#endif
	
	VG_(track_start_client_code)	(switch_thread);
	VG_(track_pre_thread_ll_exit)	(thread_exit);
	VG_(track_pre_deliver_signal)	(signal);
	
	VG_(clo_vex_control).iropt_unroll_thresh = 0;
	VG_(clo_vex_control).guest_chase_thresh  = 0;
}

VG_DETERMINE_INTERFACE_VERSION(pre_clo_init)
