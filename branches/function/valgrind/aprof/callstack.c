#include "aprof.h"

#if TRACE_FUNCTION

/* Global vars */

/* default object */
char * anon_obj = "UNKNOWN";
/* 
 * _ld_runtime_resolve need a special handling,
 * we need to know its address and length
 * 
 * runtime_resolve_addr == 1 means that ld is not stripped,
 * so never have to search for its code in an object
 */
static Addr runtime_resolve_addr   = 0;
static int  runtime_resolve_length = 0;
/* HT of all BB */
static HashTable * bb_ht = NULL;
/* HT of all function */
static HashTable * fn_ht = NULL;

#define N_FN_ENTRIES  87
#define N_OBJ_ENTRIES 47
#define HASH_CONSTANT 256

/* End global vars */

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

static UInt str_hash(const Char *s, UInt table_size)
{
    int hash_value = 0;
    for ( ; *s; s++)
        hash_value = 31 * hash_value + *s;
        
    if (hash_value == 0) {
		hash_value = 1;
		VG_(printf)("Function %s has hash zero\n");
    }
    return hash_value;
}

/* End callgrind function */

/* Init the stack */
static void init_stack(ThreadData * tdata) {
	
	if (bb_ht == NULL || fn_ht == NULL) {
		
		bb_ht = HT_construct(NULL);
		if (bb_ht == NULL) failure("bb ht not allocable");
		fn_ht = HT_construct(NULL);
		if (fn_ht == NULL) failure("fn ht not allocable");
		
	}
	
	if (tdata->stack_real == NULL) {
		
		tdata->stack_real = VG_(calloc)("stack", sizeof(act_stack), 1);
		if (tdata->stack_real == NULL) failure("Stack not allocable");
		
	}
	
	tdata->stack_real->head = VG_(calloc)("thread_data", sizeof(activation) * 4096, 1);
	if (tdata->stack_real->head == NULL) failure("Stack not allocable");
	
	tdata->stack_real->current = tdata->stack_real->head;
	tdata->stack_real->depth = 0;

}

/* Push a new activation on the stack */
static void push_stack(ThreadData * tdata, act_stack * stack, UWord sp, 
							UWord addr, Function * fn, VgSectKind section) 
{
	
	stack->depth++;
	stack->current++;
	stack->current->sp = sp;
	stack->current->ret_addr = 0;
	
	if (fn != NULL) {
		
		stack->current->hash_fn = fn->hash;
		function_enter(tdata, fn->hash, fn->name, fn->obj);
		
	} else {
		
		stack->current->hash_fn = 0;
		
	}
	
	
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
static UWord pop_stack(ThreadData * tdata, act_stack * stack, UWord csp, UWord n_frames) {
	
	//VG_(printf)("POP\n");
	
	UWord n_pop = 0;
	
	/*
	if (n_frames >= stack.depth)
		failure("Too much pop request!");
	*/
	
	while (	stack->depth > 0 && 
			(
				csp > stack->current->sp || 
				(csp == stack->current->sp && n_frames > 0)
			)
		  ) 
	{
		
		if (stack->current->hash_fn > 0) {
			function_exit(tdata, stack->current->hash_fn);
		}
		
		if (n_frames > 0) n_frames--;
		n_pop++;
		/* Adjust stack */
		stack->depth--;
		stack->current--;
		
	}
	
	return n_pop;
	
}

/* Helper function called at start of a BB */
VG_REGPARM(2) void BB_start(UWord target, UWord type_op) {

	/* Get thread data */
	ThreadId tid = thread_running();
	ThreadData * tdata = get_thread_data(tid);
	
	/* Previoud BB executed */
	BB * last_bb = tdata->last_bb;
	enum jump_t exit = NONE;
	if (last_bb != NULL) {
		exit = last_bb->exit;
	}
	
	/* Init stack if needed */
	if (tdata->stack_real == NULL) 
		init_stack(tdata);
	
	/* Stack for current thread */
	act_stack * stack = tdata->stack_real;

	/* Obtain current stack pointer */
	UWord csp = (UWord) VG_(get_SP)(1);

	/* Get info about this BB */
	BB * bb = HT_lookup(bb_ht, target, NULL); 
	
	Bool different_obj = False;
	Bool different_sect = False;
	
	/* Create a new BB structure */
	if (bb == NULL) {
		
		bb = VG_(calloc)("bb", sizeof(BB), 1);
		if (bb == NULL) failure("BB not allocable");
		
		bb->addr = target;
		
		bb->fn = VG_(calloc)("fn", sizeof(Function), 1);
		
		/* Function name buffer */
		char * fn = VG_(calloc)("fn_name", 256, 1);
		/* 
		 * Are we entering a new function? Ask to Valgrind
		 * if this BB is the first of a function. Valgrind sometimes
		 * does not know this information, so it return False. If this is
		 * the case, we try different stategies to capture a call, see
		 * later.
		 */
		bb->is_entry = VG_(get_fnname_if_entry)(target, fn, 256);
		/* If is not entry, we need anyway info about this function */
		Bool info_fn = True; 
		if (!bb->is_entry) {
			info_fn = VG_(get_fnname)(target, fn, 256);
		}
		
		if (info_fn && VG_(strcmp)(fn, "_dl_runtime_resolve") == 0) {
			bb->is_dl_runtime_resolve = 1;
			runtime_resolve_addr = 1; /* this only means that ld is not stripped */
		}
		
		Function * f = NULL;
		UInt hash = 0;
		if (info_fn) {
			hash = str_hash(fn, N_FN_ENTRIES);
			f = HT_lookup(fn_ht, hash, NULL);
		}
		
		char * obj_name = NULL;
		if (f == NULL || f->obj == 0) {
			
			/* Obtain debug info about this BB */
			DebugInfo * di = VG_(find_DebugInfo)(target);
			/* Obtain object name */
			obj_name = di ? VG_(strdup)("obj_name",
								VG_(DebugInfo_get_filename)(di)) 
								: NULL;
			if (obj_name == NULL)
				obj_name = anon_obj;
			
			bb->obj_hash = str_hash(obj_name, N_OBJ_ENTRIES); 
			
			if (last_bb != NULL && bb->obj_hash != last_bb->obj_hash)
				different_obj = True;
			
			/* try to see if we find dl_runtime_resolve in this obj */
			if (!bb->is_dl_runtime_resolve && 
				different_obj && runtime_resolve_addr == 0) {
			
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
			!bb->is_dl_runtime_resolve && runtime_resolve_addr > 1 &&
			(target >= runtime_resolve_addr) &&
			(target < runtime_resolve_addr + runtime_resolve_length)
			) 
		{
			bb->is_dl_runtime_resolve = True;
			VG_(sprintf)(fn, "_dl_runtime_resolve");
			info_fn = True;
		}
		
		if (info_fn && f == NULL) {
			
			UInt hash_2 = str_hash(fn, N_FN_ENTRIES);
			if (hash != hash_2) {
				f = HT_lookup(fn_ht, hash, NULL);
				hash = hash_2;
			}
			
			if (f == NULL) {
				
				f = VG_(calloc)("fn", sizeof(Function), 1);
				if (f == NULL) failure("Function not allocable");
				
				f->name = fn;
				HT_add_node(fn_ht, hash, f);
				
				if (obj_name != anon_obj)
					f->obj = VG_(strdup)("obj_name", obj_name);
				
				f->hash = hash;
			}
			
		}
		bb->fn = f;
		
		/* Obtain section kind of this BB */
		bb->obj_section = VG_(DebugInfo_sect_kind)(NULL, 0, target);
		
		if (last_bb != NULL && bb->obj_section != last_bb->obj_section)
			different_sect = True;
		
		bb->instr_offset = 0; /* real value filled with BB_end */
		
		HT_add_node(bb_ht, target, bb);
	
	}

	/* Are we converting RET into a CALL? */
	Bool ret_as_call = False;
	
	/* Estimation of number of returned functions */ 
	UWord n_pop = 1;
	
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
	if (exit == BBRET) {
		
		/* This is a call! */
		if (csp < stack->current->sp) {
			n_pop = 0;
			//VG_(printf)("On BB %lu, RET to CALL because SP(%lu) < TOP_SP(%lu)\n", target, csp, stack.current->sp);
		}
		/* 
		 * SP does not change, so this is a call only if return 
		 * does not match our expected return address.
		 */
		else if (csp == stack->current->sp) {
			
			//VG_(printf)("Try to convert RET to CALL for BB(%lu) because SP=CSP=%lu\n", target, csp);
			
			activation * c = stack->current;
			UWord depth = stack->depth;
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
			exit = BBOTHER;
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
	if (last_bb != NULL && exit != BBCALL && exit != BBRET)
	{
		
		if (ret_as_call || different_obj || different_sect || bb->is_entry) {
			
			/* 
			 * if the last BB is dl_runtime_resolve, we have to 
			 * do a pop in out stack. If we avoid this, then
			 * we see that dl_runtime_resolve call the resolved
			 * function (this not make sense! It resolve and
			 * return to the caller).
			 */
			if (stack->depth > 0 && last_bb->is_dl_runtime_resolve) {
				
				//VG_(printf)("POP caused by dl_runtime_resolve\n");
				
				/* Update current stack pointer */
				csp = stack->current->sp;
				
				if (stack->current->hash_fn > 0) {
					function_exit(tdata, stack->current->hash_fn);
				}
				
				/* Adjust stack */
				stack->depth--;
				stack->current--;
				
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
		
		pop_stack(tdata, stack, csp, n_pop);
		
	} else {
		
		/* 
		 * By definition, if this is a call, SP must be equal or smaller
		 * of SP on top of our stack. Try to see if this is not true...
		 */
		n_pop = pop_stack(tdata, stack, csp, 0);
		if (n_pop > 0) {
			//VG_(printf)("Call deleted\n");
			exit = BBRET;
		} 
		
		if (exit == BBCALL) {
			
			/* I don't know why we do this but callgrind does so... */
			if (call_emulation && stack->depth > 0)
				csp = stack->current->sp;
			
			if (last_bb->exit == BBCALL)
				stack->current->ret_addr = last_bb->addr + last_bb->instr_offset;
			
			push_stack(tdata, stack, csp, bb->addr, bb->fn, bb->obj_section); 
			
		}
		
	}
	
	#if 0
	/* Safety check */
	if (
		(bb->fn == NULL && stack->current->hash_fn > 0) ||
		(bb->fn->hash != stack->current->hash_fn)
		)
	{
		//VG_(printf)("Valgrind says you are in %s() but simulated stack says %s()\n", fn, stack.current->name);
		print_stack();
		failure("Mismatch");
	}
	#endif
	
	/* Update pointer last BB executed */
	tdata->last_bb = bb;
	
	/* Reset exit of current BB */
	tdata->last_bb->exit = BBOTHER;

}

/* 
 * Helper function called at the end of a BB (so it's executed only
 * if there is not a taken jump within the BB) 
 */
VG_REGPARM(3) void BB_end(UWord target, UWord type_op, UWord instr_offset)
{

	ThreadData * tdata = get_thread_data(0);

	tdata->last_bb->exit = type_op;
	if (type_op == BBCALL) 
		tdata->last_bb->instr_offset = instr_offset;

}

#endif
