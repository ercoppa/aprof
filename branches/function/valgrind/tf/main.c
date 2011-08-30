
/*--------------------------------------------------------------------*/
/*--- TF: function tracing tool                             main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of TF, a minimal Valgrind tool,
   which try to trace function entry/exit.

   Copyright (C) 2002-2010 Nicholas Nethercote
      njn@valgrind.org

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

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_xarray.h"
#include "pub_tool_clientstate.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_options.h"
#include "pub_tool_machine.h"
#include "pub_tool_vki.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_threadstate.h"
#include "valgrind.h"
#include "pub_tool_mallocfree.h"

typedef struct act {
	UWord sp;
	UWord target;
	Char * fn;
	Bool simul;
} act;

static UWord stack_depth = 0;
static act * current = NULL; 
static act * tstack = NULL;

static UWord last_bb_jump = 0;
static UWord last_bb_ret  = 0;
static VgSectKind last_bb_sect_kind  = 0;
static UChar * last_bb_obj_name = 0;
static Bool last_bb_info_sect_obj = False;
static Char * anonymous_obj = "???";

typedef enum jump_t { BB_INIT, CALL, RET, BBCALL, BBRET, BOR, BBBOR } jump_t;

static void failure(char * msg) {

	VG_(printf)("%s\n", msg);
	VG_(exit)(1);

}

static void print_stack(void) {
	UWord depth = stack_depth;
	act * c = current;
	while(stack_depth > 0) {
		
		VG_(printf)("[%lu] %s()\n", depth, c->fn);
		c--;
		depth--;
	}
}


static void function_enter(UWord target, char * name) {
	int i = 0;
	for(i = 0; i < stack_depth; i++)
		VG_(printf)("| ");
	VG_(printf)("> %s()\n", name);
	//VG_(printf)("[%lu] Entered %s()\n", stack_depth, name);
}

static void function_exit(UWord target, char * name) {
	return;
}

static void init_stack(UWord csp, UWord target) {
	
	tstack = VG_(calloc)("thread_data", sizeof(act) * 4096, 1);
	current = tstack;
	/* Function name buffer */
	Char * fn = VG_(calloc)("fn name", 256, 1);
	/* valgrind call the init function "(below main)" */
	VG_(sprintf)(fn, "(below main)");
	current->fn = fn;
	current->sp = csp;
	current->target = 0; /* Fake target */
	
	/* Safety check: we never execute the first 
	 * instruction of this function */
	if (VG_(get_fnname_if_entry)(target, fn, 256)) failure("Wrong");
	
}

static VG_REGPARM(2) void call(UWord target, UWord type_op) {

	DebugInfo * di = VG_(find_DebugInfo)(target);
	UChar * obj_name = di ? (Char*) VG_(DebugInfo_get_filename)(di) : anonymous_obj;
	VgSectKind sect_kind = VG_(DebugInfo_sect_kind)(NULL, 0, target);

	Bool different_ELF_section = False;
	if (
			last_bb_info_sect_obj && 
			(
				VG_(strcmp)(obj_name, last_bb_obj_name) != 0 ||
				sect_kind != last_bb_sect_kind
			)
		)
	{
		different_ELF_section = True;
	}
	
	last_bb_obj_name = obj_name;
	last_bb_sect_kind = sect_kind;

	/* Obtain current stack pointer */
	UWord csp = (UWord) VG_(get_SP)(1);

	/* Init stack */
	if (tstack == NULL) {
		init_stack(csp, target);
		return;
	}


	Bool simulate_call = False;
	/* Function name buffer */
	Char * fn = VG_(calloc)("fn name", 256, 1);
	/* Are we entering a new function? */
	Bool call_fn = VG_(get_fnname_if_entry)(target, fn, VG_(strlen)(fn));
	if (!call_fn && last_bb_jump != 0) {
		VG_(sprintf)(fn, "%p", (void *) target);
		last_bb_jump = 0;
		call_fn = True;
		simulate_call = True;
	} 
	/* If not, ask valgrind where we are... */
	Bool act_fn = True;
	if (!call_fn)
		act_fn = VG_(get_fnname)(target, fn, VG_(strlen)(fn));
	
	if (!act_fn) {
		/* 
		 * Why is this happening?
		 * I don't know!
		 */
		VG_(printf)("Valgrind does not know where we are: %p\n", (void *)target);
	}
	

	if (call_fn) {
		
		/* Before the call, some functions are returned? */
		while(current->sp < csp && stack_depth > 0) {
			
			//VG_(printf)("We miss a return :(\n");
			
			/* This function is returned */
			function_exit(current->target, current->fn);
			/* Adjust stack */
			stack_depth--;
			current--;
			
		}
		
		/* Register the new activation of a function */
		stack_depth++;
		current++;
		current->sp = csp;
		current->target = target;
		current->fn = fn;
		current->simul = simulate_call;
		function_enter(current->target, current->fn);
		
	} else if (csp > current->sp) {
	
		/* One or more function returned */
		while(stack_depth > 0) {
			
			function_exit(current->target, current->fn);
			stack_depth--;
			current--;
			
			if (csp <= current->sp) {
				
				//VG_(printf)("[%lu] Inside %s\n", stack_depth, current->fn);
				
				/* Safety check */
				if (act_fn && VG_(strcmp)(current->fn,fn) != 0) {
					
					if (target == current->target) {
						
						VG_(free)(current->fn);
						current->fn = fn;
						break;
						
					}
					
					if (last_bb_ret && current->simul && stack_depth > 0) {
						stack_depth--;
						current--;
						if (VG_(strcmp)(current->fn,fn) == 0) break;
					}
					
					VG_(printf)("Simulated stack says you are in %s but valgrind says %s\n", current->fn, fn);
					failure("Mismatch during return\n");
				}
				
				break;
			}
		}
		
	} 
	
	last_bb_ret = 0;

}

static VG_REGPARM(2) void call_fin(UWord target, UWord type_op) {
	if (type_op == BBRET) last_bb_ret = target;
	else if (type_op == BBCALL) last_bb_jump = target;
	else last_bb_info_sect_obj = True;
}

static
IRSB* tf_instrument (  VgCallbackClosure* closure, 
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
	
	IRExpr  * e1 = mkIRExpr_HWord ( BB_INIT );
	IRExpr  * e2 = mkIRExpr_HWord ( (HWord) sbIn->stmts[i]->Ist.IMark.addr );
	IRDirty * di2 = unsafeIRDirty_0_N( 2, "call",
								VG_(fnptr_to_fnentry)( &call ),
								mkIRExprVec_2( e2, e1 ) );

	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	
	for (/*use current i*/; i < sbIn->stmts_used; i++) {
		
		IRStmt* st = sbIn->stmts[i];
		if (!st || st->tag == Ist_NoOp) continue;

		switch (st->tag) {

			case Ist_NoOp:
			case Ist_AbiHint:
			case Ist_Put:
			case Ist_PutI:
			case Ist_MBE:
			case Ist_IMark:
			case Ist_WrTmp:
			case Ist_Store:
			case Ist_Dirty:
			case Ist_CAS:
			case Ist_LLSC: {
				
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

	
	if (sbIn->jumpkind == Ijk_Call) {
		
		e1 = mkIRExpr_HWord ( BBCALL );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call_fin",
								VG_(fnptr_to_fnentry)( &call_fin ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else if (sbIn->jumpkind == Ijk_Ret) {
		
		e1 = mkIRExpr_HWord ( BBRET );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call_fin",
								VG_(fnptr_to_fnentry)( &call_fin ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else {
		
		e1 = mkIRExpr_HWord ( BBBOR );
		IRDirty * di = unsafeIRDirty_0_N( 2, "call_fin",
								VG_(fnptr_to_fnentry)( &call_fin ),
								mkIRExprVec_2( sbIn->next, e1 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	}

	return sbOut;
}

/* aprof initialization */
static void tf_post_clo_init(void) {
	return;
}

/* Funzione per presentare risultati in fase finale */
static void tf_fini(Int exitcode) {
	return;
}

/* Valgrind init */
static void tf_pre_clo_init(void) {

	VG_(details_name)            ("TF");
	VG_(details_version)         (NULL);
	VG_(details_description)     ("function tracing tool");
	VG_(details_copyright_author)("By Emilio Coppa, thanks to Josef Weidendorfer");
	VG_(details_bug_reports_to)  ("ercoppa@gmail.com");

	VG_(basic_tool_funcs) 				(tf_post_clo_init, tf_instrument, tf_fini);
	
	VG_(clo_vex_control).iropt_level = 0;
	VG_(clo_vex_control).iropt_unroll_thresh = 0;
	VG_(clo_vex_control).guest_chase_thresh  = 0;
	
}

VG_DETERMINE_INTERFACE_VERSION(tf_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
