
/*--------------------------------------------------------------------*/
/*--- TF: function tracing tool                             main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of TF, a minimal Valgrind tool,
   which try to trace function entry/exit. This tool contains a
   more simple version of code/logic used by callgrind. It's not
   optimized, you can do better. TF is provided as toy tool that
   you can copy/modify/extend if you need function tracing.
   TF does not manage multithread process, but it's easy to fix this,
   you have to allocate a new stack for each thread and each function
   has to manipulate the appropriate stack (see 
   VG_(get_running_tid)() )
   
   Special thanks to Josef Weidendorfer (author of callgrind) 
   that helps me a lot for TF. Some code of TF is taken from callgrind.
   
   Some useful reading about function tracking and Valgrind:
   1) valgrind/docs/internals/tracking-fn-entry-exit.txt
   2) http://thread.gmane.org/gmane.comp.debugging.valgrind/11474
   3) http://thread.gmane.org/gmane.comp.debugging.valgrind/11477
   4) http://thread.gmane.org/gmane.comp.debugging.valgrind/11533
   5) http://thread.gmane.org/gmane.comp.debugging.valgrind/4962
   6) http://comments.gmane.org/gmane.comp.debugging.valgrind/11346
   
   --

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
#include "pub_tool_hashtable.h"

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
	char * name;
	/* object name */
	char * libname;
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
	/* Is this BB part of dl_runtime_resolve? */
	Bool is_dl_runtime_resolve;
	
} BB;

/* a code pattern is a list of tuples (start offset, length) */
struct chunk_t { int start, len; };
struct pattern
{
    const char* name;
    int len;
    struct chunk_t chunk[];
};

/* Global var */

/* We have to take info only about the last executed BB */
static BB last_bb;
/* Global stack */
static act_stack stack; 
/* HT of all BB that are entry point for a function */
static VgHashTable bb_entry_ht = NULL;
/* default object */
char * anon_obj = "UNKNOWN";
/* 
 * _ld_runtime_resolve need a special handling,
 * we need to know its address and length
 */
static Addr runtime_resolve_addr   = 0;
static int  runtime_resolve_length = 0;

static void failure(char * msg) {

	VG_(printf)("%s\n", msg);
	VG_(exit)(1);

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
static Bool check_code(UWord obj_start, UWord obj_size, 
						unsigned char code[], struct pattern* pat)
{
	Bool found;
	Addr addr, end;
	int chunk, start, len;

	/* 
	 * first chunk of pattern should always start at offset 0 and
	 * have at least 3 bytes 
	 */
	if (!(pat->chunk[0].start == 0) || !(pat->chunk[0].len > 2))
		failure("Check on chunk pat failed");

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

				if(!(len >2)) failure("fail check pattern");

				if (VG_(memcmp)( (void*)(addr+start), code+start, len) != 0) {
					found = False;
					break;
				}
				chunk++;
			}

			if (found) {
				
				runtime_resolve_addr   = addr;
				runtime_resolve_length = pat->len;
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
static Bool search_runtime_resolve(char * obj_name, UWord obj_start,
										UWord obj_size)
{
#if defined(VGP_x86_linux)
    static unsigned char code[] = {
	/* 0*/ 0x50, 0x51, 0x52, 0x8b, 0x54, 0x24, 0x10, 0x8b,
	/* 8*/ 0x44, 0x24, 0x0c, 0xe8, 0x70, 0x01, 0x00, 0x00,
	/*16*/ 0x5a, 0x59, 0x87, 0x04, 0x24, 0xc2, 0x08, 0x00 };
    /* Check ranges [0-11] and [16-23] ([12-15] is an absolute address) */
    static struct pattern pat = {
	"x86-def", 24, {{ 0,12 }, { 16,8 }, { 24,0}} };

    /* Pattern for glibc-2.8 on OpenSuse11.0 */
    static unsigned char code_28[] = {
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
    static unsigned char code[] = {
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
    static unsigned char code[] = {
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

/* debug function for printing the stack */
static void print_stack(void) {
	
	VG_(printf)("\nStack trace:\n");
	VG_(printf)("\nCurren stack poiter: %lu\n",  (UWord) VG_(get_SP)(1));
	
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
static void function_enter(UWord target, char * name, VgSectKind section) {
	
	int i = 0;
	for(i = 0; i < stack.depth -1; i++)
		VG_(printf)("| ");

	char * sect = "";
	if (section == Vg_SectPLT) sect = "[PLT]";
	
	//char * sect = (char *)VG_(pp_SectKind)(section);
	
	if (name == NULL) VG_(printf)("> %p %s\n", target, sect);
	else VG_(printf)("> %s\n", name);
}

/* Called when a function return */
static void function_exit(UWord target, char * name) {
	return;
}

static Bool BB_is_entry(UWord target) {
	
	void * n = VG_(HT_lookup) (bb_entry_ht, target);
	if (n == NULL) return False;
	
	return True;
	
}

static void BB_is_entry_add(UWord target) {
	
	if (bb_entry_ht == NULL) {
		bb_entry_ht = VG_(HT_construct)("bb entry");
		if (bb_entry_ht == NULL) failure("bb entry ht not allocable");
	}
	
	if (!BB_is_entry(target)) {
		VgHashNode * n = VG_(calloc)("ht node", sizeof(VgHashNode), 1);
		if (n == NULL) failure("ht node not allocable");
		
		n->key = target;
		VG_(HT_add_node)(bb_entry_ht, n);
	}
	
}

/* Init the stack */
static void init_stack(UWord csp, UWord target) {
	
	stack.head = VG_(calloc)("thread_data", sizeof(activation) * 4096, 1);
	if (stack.head == NULL) failure("Stack not allocable");
	stack.current = stack.head;
	stack.depth = 0;
	
	return;
	
	/* Function name buffer */
	Char * fn = VG_(calloc)("fn name", 256, 1);
	/* valgrind call the init function "(below main)" */
	VG_(sprintf)(fn, "(below main)");
	stack.current->name = fn;
	stack.current->sp = csp;
	stack.current->addr = 0; /* Fake target */
	
	/* Obtain debug info about this BB */
	DebugInfo * di = VG_(find_DebugInfo)(target);
	/* Obtain object name */
	char * obj_name = di ? (Char*) VG_(strdup)("obj_name",
								VG_(DebugInfo_get_filename)(di))
							: NULL;
	if (obj_name == NULL) obj_name = anon_obj;
	
	/* try to see if we find dl_runtime_resolve in this obj */
	if (runtime_resolve_addr == 0) {
		UWord obj_start = di ? VG_(DebugInfo_get_text_avma)(di) : 0;
		UWord obj_size = di ? VG_(DebugInfo_get_text_size)(di) : 0;
		search_runtime_resolve(obj_name, obj_start, obj_size);
	}
	
	/* Obtain section kind of this BB */
	VgSectKind sect_kind = VG_(DebugInfo_sect_kind)(NULL, 0, target);
	
	last_bb.addr = target;
	last_bb.instr_offset = 0;
	last_bb.obj_name = obj_name;
	last_bb.obj_section = sect_kind;
	last_bb.exit = NONE;
	
	/* Safety check: we never execute the first 
	 * instruction of this function */
	if (VG_(get_fnname_if_entry)(target, fn, 256)) failure("Wrong");
	

}

/* Push a new activation on the stack */
static void push_stack(UWord sp, UWord target, char * name, 
						Char * libname, UWord ret, VgSectKind section) 
{
	
	stack.depth++;
	stack.current++;
	stack.current->sp = sp;
	stack.current->addr = target;
	stack.current->name = name;
	stack.current->libname = libname;
	stack.current->ret_addr = ret;
	function_enter(target, name, section);
	
	/* Safety check */
	//if (stack.depth > 0 && (stack.current-1)->sp < sp)
	//	failure("Caller has a lower SP!");
	
	//VG_(printf)("PUSH: addr %lu - depth %lu - SP %lu\n", target, stack.depth, sp);
	//VG_(printf)("PUSH: addr %lu\n", target);
	
	//if (stack.depth > 10) failure("Depth too much");

}

/* 
 * Sync stack with current SP (CSP), if n_frames > 0
 * and CSP == SP, pop out n_frames activation.
 * Return how many activations are removed from the stack.
 */
static UWord pop_stack(UWord csp, UWord n_frames) {
	
	//VG_(printf)("POP\n");
	
	UWord n_pop = 0;
	
	/*
	if (n_frames >= stack.depth)
		failure("Too much pop request!");
	*/
	
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
		if (stack.current->name != NULL) 
			VG_(free)(stack.current->name);
		if ((char *)stack.current->libname != (char *) anon_obj)
			VG_(free)(stack.current->libname);
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
		//return;
	}

	/* Obtain debug info about this BB */
	DebugInfo * di = VG_(find_DebugInfo)(target);
	/* Obtain object name */
	char * obj_name = di ? VG_(strdup)("obj_name",
								VG_(DebugInfo_get_filename)(di)) 
							: NULL;
	if (obj_name == NULL) obj_name = anon_obj;
    
	/* Compare object of this BB with last BB */
	Bool different_obj = False;
	if (VG_(strcmp)(obj_name, last_bb.obj_name) != 0)
		different_obj = True;

	/* Obtain section kind of this BB */
	VgSectKind sect_kind = VG_(DebugInfo_sect_kind)(NULL, 0, target);
	/* Compare section of this BB with last BB */
	Bool different_sect = False;
	if (sect_kind != last_bb.obj_section)
		 different_sect = True;
	
	/* try to see if we find dl_runtime_resolve in this obj */
	if (runtime_resolve_addr == 0 && different_obj) {
		
		/* Obtain obj start address */
		UWord obj_start = di ? VG_(DebugInfo_get_text_avma)(di) : 0;
		/* Obtaind obj size */
		UWord obj_size = di ? VG_(DebugInfo_get_text_size)(di) : 0;
		/* Check */
		search_runtime_resolve(obj_name, obj_start, obj_size);
	
	}

	/* Are we converting RET into a CALL? */
	Bool ret_as_call = False;
	
	/* Function name buffer */
	char * fn = VG_(calloc)("fn_name", 256, 1);
	/* 
	 * Are we entering a new function? Ask to Valgrind
	 * if this BB is the first of a function. Valgrind sometimes
	 * does not know this information, so it return False. If this is
	 * the case, we try different stategies to capture a call, see
	 * later.
	 */
	Bool is_entry = VG_(get_fnname_if_entry)(target, fn, 256);
	/* If not, ask valgrind where we are... */
	Bool info_fn = True; /* Have we info about current fn? */
	if (!is_entry) {
		info_fn = VG_(get_fnname)(target, fn, 256);
	} else {
		BB_is_entry_add(target);
	}
	
	Bool is_dl_runtime_resolve = False;
	/* Check if this BB is dl_runtime_resolve */
	if (	
			(info_fn && VG_(strcmp)(fn, "_dl_runtime_resolve") == 0)
			||
			(
			runtime_resolve_addr && 
			(target >= runtime_resolve_addr) &&
			(target < runtime_resolve_addr + runtime_resolve_length)
			)
		)
	{
		/* BB in runtime_resolve found by code check; use this name */
		if (!info_fn) {
			VG_(sprintf)(fn, "_dl_runtime_resolve");
			info_fn = True;
		}
		/*
		 * Jump at end into the resolved function should not be
		 * represented as a call, but as a return + call.
		 */
		is_dl_runtime_resolve = True;
		
		//VG_(printf)("Found dl_runtime_resolve\n");
		
    }
	
	/* Estimation of number of returned functions */ 
	UWord n_pop = 1;
	
	if (last_bb.exit == BBCALL) {
		
		/*
		 * if this function returns, we expect as return address this:
		 */
		stack.current->ret_addr = last_bb.addr + last_bb.instr_offset;
		//VG_(printf)("Ret address: %lu : %lu + %lu\n", stack.current->ret_addr,
		//	last_bb.addr, last_bb.instr_offset);

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
		if (csp < stack.current->sp) {
			n_pop = 0;
			//VG_(printf)("On BB %lu, RET to CALL because SP(%lu) < TOP_SP(%lu)\n", target, csp, stack.current->sp);
		}
		/* 
		 * SP does not change, so this is a call only if return 
		 * does not match our expected return address.
		 */
		else if (csp == stack.current->sp) {
			
			//VG_(printf)("Try to convert RET to CALL for BB(%lu) because SP=CSP=%lu\n", target, csp);
			
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
				//VG_(printf)("No match found BB(%lu)\n", target);
				break;
			}
			
		}
		
		if (n_pop == 0) {
			ret_as_call = True;
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
	Bool call_emulation = False;
	if (last_bb.addr > 0 && last_bb.exit != BBCALL && last_bb.exit != BBRET)
	{
		
		if (ret_as_call || different_obj || different_sect || BB_is_entry(target)) {
			
			/* 
			 * if the last BB is dl_runtime_resolve, we have to 
			 * do a pop in out stack. If we avoid this, then
			 * we see that dl_runtime_resolve call the resolved
			 * function (this not make sense! It resolve and
			 * return to the caller).
			 */
			if (stack.depth > 0 && last_bb.is_dl_runtime_resolve) {
				
				//VG_(printf)("POP caused by dl_runtime_resolve\n");
				
				/* Update current stack pointer */
				csp = stack.current->sp;
				
				function_exit(stack.current->addr, stack.current->name);
				if (stack.current->name != NULL)
					VG_(free)(stack.current->name);
				if ((char *)stack.current->libname != (char *) anon_obj)
					VG_(free)(stack.current->libname);
				/* Adjust stack */
				stack.depth--;
				stack.current--;
				
			}
			
			last_bb.exit = BBCALL;
			
			/* 
			 * We are simulating a call... later we have to fix 
			 * something in the stack
			 */
			call_emulation = True;
			
		}
		
	}
	
	/* 
	 * Ok, now it's time to adjust stack and notify function exit or
	 * entry :)
	 */
	if (last_bb.exit == BBRET) {
		
		pop_stack(csp, n_pop);
		
	} else {
		
		/* 
		 * By definition, if this is a call, SP must be equal or smaller
		 * of SP on top of our stack. Try to see if this is not true...
		 */
		n_pop = pop_stack(csp, 0);
		if (n_pop > 0) {
			//VG_(printf)("Call deleted\n");
			last_bb.exit = BBRET;
		} 
		
		if (last_bb.exit == BBCALL) {
			
			/* I don't know why we do this but callgrind does so... */
			if (call_emulation && stack.depth > 0)
				csp = stack.current->sp;
			
			if (!info_fn) {
				VG_(free)(fn);
				fn = NULL;
			}
			
			push_stack(csp, target, fn, obj_name, 0, sect_kind); 
			
		}
		
	}
	
	/* Safety check */
	if (
			info_fn && stack.current->name != NULL &&
			VG_(strcmp)(stack.current->name, fn) != 0
		)
	{
		VG_(printf)("Valgrind says you are in %s() but simulated stack says %s()\n", fn, stack.current->name);
		print_stack();
		failure("Mismatch");
	}
	
	if (!info_fn && fn != NULL) {
		VG_(free)(fn);
		fn = NULL;
	}
	
	/* Update info about this BB */
	last_bb.addr = target;
	last_bb.instr_offset = 0;
	last_bb.obj_name = obj_name;
	last_bb.obj_section = sect_kind;
	last_bb.exit = BBOTHER;
	last_bb.is_dl_runtime_resolve = is_dl_runtime_resolve;

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
	IRExpr  * e2 = mkIRExpr_HWord ( (HWord) (Addr)sbIn->stmts[i]->Ist.IMark.addr );
	IRDirty * di2 = unsafeIRDirty_0_N( 2, "BB start",
								VG_(fnptr_to_fnentry)( &BB_start ),
								mkIRExprVec_2( e2, e1 ) );

	addStmtToIRSB( sbOut, IRStmt_Dirty(di2) );
	
	for (/*use stack. i*/; i < sbIn->stmts_used; i++) {
		
		IRStmt* st = sbIn->stmts[i];
		if (!st || st->tag == Ist_NoOp) continue;

		switch (st->tag) {

			case Ist_IMark: {
				
				if (st->Ist.IMark.len == 0)
					instr_offset += VG_MIN_INSTR_SZB;
				else
					instr_offset += st->Ist.IMark.len;
					
				//VG_(printf)("Instruction size: %lu\n", st->Ist.IMark.len);
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

	//VG_(printf)("Instr size(%lu): %lu\n", bb_addr, instr_offset);

	return sbOut;
}

/* aprof initialization */
static void tf_post_clo_init(void) {
	last_bb.addr = 0;
	last_bb.exit = BBOTHER;
	last_bb.instr_offset = 0;
	last_bb.obj_name =  anon_obj;
	last_bb.obj_section = Vg_SectUnknown;
	
	VG_(printf)("\n\n");
	
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
	
	//VG_(clo_vex_control).iropt_level = 0;
	VG_(clo_vex_control).iropt_unroll_thresh = 0;
	VG_(clo_vex_control).guest_chase_thresh = 0;
	
}

VG_DETERMINE_INTERFACE_VERSION(tf_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
