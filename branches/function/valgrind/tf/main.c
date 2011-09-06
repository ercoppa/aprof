
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

/* Activation record on the stack */
typedef struct activation {
	/* Stack pointer when entered this function */
	UWord sp;
	/* address of first function BB */
	UWord addr;
	/* 
	 * If A() call  B() (and BB of A has as jumpkind Ijk_Call),
	 * when B() returned, I will expect to execute the BB of A() 
	 * with this address.
	 */
	UWord ret_addr;
	/* Function name */
	Char * name;
	/* lib name */
	Char * libname;
} activation;

typedef struct act_stack {
	UWord depth;          /* stack depth */
	activation * head;    /* head of the stack */
	activation * current; /* current act */
} act_stack;

/* 
 * When I call an helper function during the exec of a BB,
 * I descriminate the point with one of these: 
 */
typedef enum jump_t { 
	BB_INIT,                /* head of BB */
	CALL, RET, OTHER,       /* jump within a BB */
	BBCALL, BBRET, BBOTHER, /* tail of BB */
	NONE                    /* default value */
} jump_t;

/* Info about a Basic Block */
typedef struct BB {
	
	/* Basic bloc address */
	UWord addr;
	/* length of BB, valid only if BB_end is exec by this BB with BBCALL */
	UWord instr_offset;
	/* 
	 * Type of exit (jumpkind of the BB):
	 * - if Ijk_Call then BBCALL
	 * - if Ijk_Ret then BBRET
	 * - if Ijk_Boring then BBOTHER
	 * If the BB never take the "final exit" then NONE
	 */
	jump_t exit;
	/* Object name (of the function) */
	UChar * obj_name;
	/* Object section (of the function) */
	VgSectKind obj_section;
	
} BB;

/* Global var */

/* We have to take info only about the last executed BB */
static BB last_bb;
/* Global stack */
static act_stack stack; 
/* unknown object */
char * anon_obj = "UNKNOW";

static void failure(char * msg) {

	VG_(printf)("%s\n", msg);
	VG_(exit)(1);

}

/* debug function for printing the stack */
static void print_stack(void) {
	
	VG_(printf)("\nStack trace:\n");
	VG_(printf)("\nstack. stack poiter: %lu\n",  (UWord) VG_(get_SP)(1));
	
	UWord depth = stack.depth;
	activation * c = stack.current;
	while(depth > 0) {
		
		VG_(printf)("[%lu] %s() sp=%lu\n", depth, c->name, c->sp);
		c--;
		depth--;
	}

	VG_(printf)("\n");

}

/* Called when a function is called */
static void function_enter(UWord target, char * name) {
	int i = 0;
	for(i = 0; i < stack.depth; i++)
		VG_(printf)("| ");
	if (name == NULL) VG_(printf)("> %p()\n", (void *)target);
	else VG_(printf)("> %s()\n", name);
}

/* Called when a function return */
static void function_exit(UWord target, char * name) {
	return;
}

static void BB_is_entry_add(UWord target) {
	return;
}
static Bool BB_is_entry(UWord target) {
	return False;
}

/* Init the stack */
static void init_stack(UWord csp, UWord target) {
	
	stack.head = VG_(calloc)("thread_data", sizeof(activation) * 4096, 1);
	if (stack.head == NULL) failure("Stack not allocable");
	stack.current = stack.head;
	stack.depth = 1;
	/* Function name buffer */
	Char * fn = VG_(calloc)("fn name", 256, 1);
	/* valgrind call the init function "(below main)" */
	VG_(sprintf)(fn, "(below main)");
	stack.current->name = fn;
	stack.current->sp = csp;
	stack.current->addr = 0; /* Fake target */
	
	/* Safety check: we never execute the first 
	 * instruction of this function */
	if (VG_(get_fnname_if_entry)(target, fn, 256)) failure("Wrong");
	
}

/* Push a new activation on the stack */
static void push_stack(UWord sp, UWord target, Char * name, 
						Char * libname, UWord ret) 
{
	
	stack.depth++;
	stack.current++;
	stack.current->sp = sp;
	stack.current->addr = target;
	stack.current->name = name;
	stack.current->libname = libname;
	stack.current->ret_addr = ret;
	function_enter(target, name);
	
	//if (stack.depth > 10) failure("Depth too much");

}

/* 
 * Sync stack with current SP (CSP), if n_frames > 0
 * and CSP == SP, pop out n_frames activation.
 * Return how many activations are removed from the stack.
 */
static UWord pop_stack(UWord csp, UWord n_frames) {
	
	UWord n_pop = 0;
	
	if (n_frames >= stack.depth)
		failure("Too much pop request!");
	
	while (	stack.depth > 0 && 
			(
				csp > stack.current->sp || 
				(csp == stack.current->sp && n_frames > 0)
			)
		  ) 
	{
		
		function_exit(stack.current->addr, stack.current->name);
		if (n_frames > 0) n_frames--;
		n_pop++;
		/* Adjust stack */
		stack.depth--;
		stack.current--;
		
	}
	
	return n_pop;
	
}

/* Helper function called at start of a BB */
static VG_REGPARM(2) void BB_start(UWord target, UWord type_op) {

	/* Obtain current stack pointer */
	UWord csp = (UWord) VG_(get_SP)(1);

	/* Init stack if needed */
	if (stack.head == NULL) {
		init_stack(csp, target);
		return;
	}

	/* Obtain debug info about this BB */
	DebugInfo * di = VG_(find_DebugInfo)(target);
	/* Obtain object name */
	UChar * obj_name = NULL;
	if (di != NULL) 
		obj_name = (Char*) VG_(strdup)("obj_name",VG_(DebugInfo_get_filename)(di));
	if (obj_name == NULL)
		obj_name = anon_obj;
	
	/* Obtain section kind of this BB */
	VgSectKind sect_kind = VG_(DebugInfo_sect_kind)(NULL, 0, target);
	
	/* Compare object/section of this BB with last BB */
	Bool different_ELF_section = False;
	if (	VG_(strcmp)(obj_name, last_bb.obj_name) != 0 ||
			sect_kind != last_bb.obj_section
		) 
	{
		different_ELF_section = True;
	}

	/* Are we simulating a call? */
	Bool simulate_call = False;
	
	/* Function name buffer */
	Char * fn = VG_(calloc)("fn name", 256, 1);
	/* 
	 * Are we entering a new function? Ask to Valgrind
	 * if this BB is the first of a function. Valgrind sometimes
	 * does not know this information, so it return False. If this is
	 * the case, we try different stategies to capture a call, see
	 * simulate_call later.
	 */
	simulate_call = VG_(get_fnname_if_entry)(target, fn, VG_(strlen)(fn));
	/* If not, ask valgrind where we are... */
	Bool info_fn = True; /* Have we info about current fn? */
	if (!simulate_call)
		info_fn = VG_(get_fnname)(target, fn, VG_(strlen)(fn));
	
	/* Estimation of number of returned functions */ 
	UWord n_pop = 0;
	/* Expected return address (when returning to the previous fn) */
	UWord ret = 0;
	if (last_bb.exit == BBCALL) {
		
		/*
		 * if this function call another function, then
		 * we expect this return address:
		 */
		ret = last_bb.addr + last_bb.instr_offset;
		
		/* 
		 * If last BB do a Ijk_Call exit but Valgrind does not 
		 * recognize this BB as first of a function, we "simulate"
		 * a call
		 */
		simulate_call = True;
		
		//VG_(printf)("Call because BBCALL\n");

	} 

	/* 
	 * If last BB do an Ijk_Ret exit not always this is a real return.
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
	else if (last_bb.exit == BBRET) {
		
		/* This is a call! */
		if (csp < stack.current->sp) n_pop = 0;
		/* 
		 * SP does not change, so this is a call only if return 
		 * does not match our expected return address.
		 */
		else if (csp == stack.current->sp) {
			
			activation * c = stack.current;
			UWord depth = stack.depth;
			while(1) {
				
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
			simulate_call = True;
			last_bb.exit = BBOTHER;
			//VG_(printf)("Call because RET as CALL\n");
		}
		
	}
	
	/* This is a call also if last BB exit is
	 * not Ijk_Call/Ijk_Ret and:
	 * - current BB is in different ELF object or section
	 * - current BB is first BB of a function (we know this from
	 *   previous info given by Valgrind) 
	 */
	if (!simulate_call && last_bb.exit != BBCALL 
			&& last_bb.exit != BBRET)
	{
		
		if (different_ELF_section || BB_is_entry(target)) {
			simulate_call = True;
			//VG_(printf)("Call because different ELF/section\n");
		}
		
	}
	
	/* 
	 * Ok, now it's time to adjust stack and notify function exit or
	 * entry :)
	 */
	if (last_bb.exit == BBRET) {
		
		pop_stack(csp, n_pop);
		
	} else if (simulate_call) {
		
		/* 
		 * By definition, if this is a call, SP must be equal or smaller
		 * of SP on top of our stack. Try to see if this is not true...
		 */
		n_pop = pop_stack(csp, 0);
		if (n_pop > 0) {
			//VG_(printf)("Call deleted\n");
			simulate_call = False;
		} else {
			
			if (!info_fn) {
				VG_(free)(fn);
				fn = NULL;
			}
			
			push_stack(csp, target, fn, obj_name, 0); 
			
		}
		
	}
	
	if (!info_fn) {
		/* 
		 * Why is this happening?
		 * I don't know!
		 */
		//VG_(printf)("Valgrind does not know where we are: %p\n", (void *)target);
	}
	
	/* Safety check */
	if (	info_fn != NULL && stack.current->name != NULL &&
			VG_(strcmp)(stack.current->name, fn) != 0
		)
	{
		failure("Mismatch between current function and simulated stack");
	}
	
	
	/* Update info about this BB */
	last_bb.addr = target;
	last_bb.instr_offset = 0;
	last_bb.obj_name = obj_name;
	last_bb.obj_section = sect_kind;
	last_bb.exit = NONE;

}

/* 
 * Helper function called at the end of a BB (so it's executed only
 * if there is not a taken jump within the BB) 
 */
static VG_REGPARM(3) void BB_end(UWord target, UWord type_op, UWord instr_offset)
{

	last_bb.exit = type_op;
	if (type_op == BBCALL) 
		last_bb.instr_offset = instr_offset;

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
	UWord instr_offset = 0;

	if (gWordTy != hWordTy) /* We don't stack.ly support this case. */
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
	IRDirty * di2 = unsafeIRDirty_0_N( 2, "BB start",
								VG_(fnptr_to_fnentry)( &BB_start ),
								mkIRExprVec_2( e2, e1 ) );

	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	
	for (/*use stack. i*/; i < sbIn->stmts_used; i++) {
		
		IRStmt* st = sbIn->stmts[i];
		if (!st || st->tag == Ist_NoOp) continue;

		switch (st->tag) {

			case Ist_IMark: {
				instr_offset += st->Ist.IMark.len;
				addStmtToIRSB( sbOut, st );
				break;
			}

			case Ist_NoOp:
			case Ist_AbiHint:
			case Ist_Put:
			case Ist_PutI:
			case Ist_MBE:
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
		e2 = mkIRExpr_HWord ( instr_offset );
		IRDirty * di = unsafeIRDirty_0_N( 3, "BB end",
								VG_(fnptr_to_fnentry)( &BB_end ),
								mkIRExprVec_3( sbIn->next, e1, e2) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else if (sbIn->jumpkind == Ijk_Ret) {
		
		e1 = mkIRExpr_HWord ( BBRET );
		e2 = mkIRExpr_HWord ( instr_offset );
		IRDirty * di = unsafeIRDirty_0_N( 3, "BB end",
								VG_(fnptr_to_fnentry)( &BB_end ),
								mkIRExprVec_3( sbIn->next, e1, e2 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	} else {
		
		e1 = mkIRExpr_HWord ( BBOTHER );
		e2 = mkIRExpr_HWord ( instr_offset );
		IRDirty * di = unsafeIRDirty_0_N( 3, "BB end",
								VG_(fnptr_to_fnentry)( &BB_end ),
								mkIRExprVec_3( sbIn->next, e1, e2 ) );
		addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
		
	}

	return sbOut;
}

/* aprof initialization */
static void tf_post_clo_init(void) {
	last_bb.addr = 0;
	last_bb.exit = NONE;
	last_bb.instr_offset = 0;
	last_bb.obj_name =  anon_obj;
	last_bb.obj_section = Vg_SectUnknown;
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
