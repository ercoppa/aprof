/*
 * aprof global header
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

#ifndef APROF_H
#define APROF_H

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
#include "pub_tool_xarray.h"
#include "pub_tool_vkiscnums.h"
#include "valgrind.h"
#include "pub_tool_mallocfree.h"
#include "hashtable/hashtable.h"
#include "../coregrind/pub_core_options.h"
#include "lookup_table.h"

#define APROF_VERSION       "0.2"
#define APROF_(str)         VGAPPEND(vgAprof_,str)
#define APROF_TOOL          1   // useful for data-common.h
#define REPORT_VERSION      6   // see documentation on our site:
                                // 1 == 1.1, 2 == 1.2, ...

/* Behavioral macro */
#define EMPTY_ANALYSIS      0   // if 1, analysis routines are empty 
                                // (useful in combination with EVENTCOUNT)

#define DEBUG               0   // Enable some sanity checks

#define VERBOSE             0   // 0 disabled, 1 function + thread, 2 
                                // function + thread + load/store/modify,
                                // 4 very verbose function tracing

#define EVENTCOUNT          0   // 0 disabled, 1 memory accesses, 
                                // 2 functions, 3 mem+fn
                                
                                // Performance metric:
#define NO_COST             0   // Nothing
#define INSTR               1   // Count guest intel instruction 
#define RDTSC               2   // rdtsc intel instruction timestamp
#define BB_COUNT            3   // count BB executed
#define COST                BB_COUNT

                                
#define MEM_TRACE           1   // if 0 disable mem instrumentation
#define THREAD_INPUT        0   // if 1, every write creates a new
                                // version of an input
#define SYSCALL_WRAPPING    0   // if 1, I/O syscall stores are 
                                // considered as external I/O

#define IGNORE_LOAD_SYS     1   // ignore load due to syscall

#define INPUT_STATS         0   // stats about thread input & external input

#define DEBUG_ALLOCATION    0   // if 1, check every allocation made by aprof
#define IGNORE_DL_RUNTIME   0   // if 1, disable analysis for dl_
                                // runtime_resolve (and its children)
                                
#define REPORT_NAME         1   // if 1 report name is prog_TID.aprof, 
                                // if 2 is PID_TID_ADDRMULTIPLE.aprof
                                
#define CCT_GRAPHIC         0   // output CCT as dot language in an 
                                // external file (file.graph); EXPERIMENTAL

#define MEM_USAGE_INFO      0   // Check VmPeak in /proc/PID/status
                                // to get info about aprof mem usage 

/* shadow memory  */

#define CHECK_OVERFLOW      1   // On 64bit machine, we map only 2048GB...

/* Some constants */
#define MAX_STACK_SIZE      15000   // max stack size
#define STACK_SIZE          64      // Initial stack size
#define BUFFER_SIZE         32000   // FILE buffer size
#define NAME_SIZE           4096    // function/object name buffer size 


#if INPUT_STATS
    #define SET_THREAD(x)  ((x) &  0x7FFFFFFF) // clear 31th bit
    #define SET_SYSCALL(x) ((x) |  0x80000000) // set 31th bit
    #define SYSCALL(x)     ((x) &  0x80000000) // mask 0-30th bits
    #define TS(x)          ((x) &  0x7FFFFFFF) // mask 31th bit
    #define MAX_COUNT_VAL  0x80000000
#else // INPUT_STATS
    #define SET_THREAD(x)  ;        // do nothing
    #define SET_SYSCALL(x) ;        // do nothing
    #define SYSCALL(x)     (False)  // False
    #define TS(x)          (x)      // do nothing
    #define MAX_COUNT_VAL  0x0
#endif // !INPUT_STATS

/* some config check */

#if defined(VG_BIGENDIAN)
    #define Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
    #define Endness Iend_LE
#else
    #error "Unknown endianness"
#endif

/* Failure/error functions */

#define vgAprof_assert(cond, ...)   do{ \
                                        if (UNLIKELY(!(cond))) { \
                                            VG_(printf)(__VA_ARGS__); \
                                            tl_assert(cond); \
                                        } \
                                    } while(0);

#if DEBUG
    #define vgAprof_debug_assert(cond, ...) do{ \
                                                if (UNLIKELY(!(cond))) { \
                                                    VG_(printf)(__VA_ARGS__); \
                                                    tl_assert(cond); \
                                                } \
                                            } while(0);
#else // DEBUG
    #define vgAprof_debug_assert(cond, ...) 
#endif // !DEBUG

#if VERBOSE > 0
    #define vgAprof_verbose(level, ...) do { \
                                            if (level <= VERBOSE)
                                                VG_(printf)(__VA_ARGS__);
                                        } while(0);
#else // VERBOSE
    #define vgAprof_verbose(level, ...)
#endif // VERBOSE

/* Data structures */

#include "data-common.h"

/* aprof runtime configuration */

extern Runtime APROF_(runtime);

/* Functions */

/* f{open, write, close, flush} internal implementation (fwrite.c) */
FILE * APROF_(fopen)(const HChar * name);
void APROF_(fflush)(FILE * f);
void APROF_(fwrite)(FILE * f, const HChar * buffer, UInt size);
void APROF_(fclose)(FILE * f);
void APROF_(fprintf)(FILE * f, const HChar * format, ...);

/* handlers of thread events (thread.c) */
void APROF_(thread_switch)(ThreadId tid, ULong blocks_dispatched);
void APROF_(thread_exit)(ThreadId tid);
void APROF_(kill_threads)(void);
void APROF_(thread_create)(ThreadId tid, ThreadId child);

/* counter overflow handlers (overflow.c) */
UInt APROF_(overflow_handler_drms)(void);
UInt APROF_(overflow_handler_rms)(void);

/* report functions (report.c) */
void APROF_(generate_report)(ThreadData * tdata, ThreadId tid);

/* CCT functions (CCT.c) */
CCTNode * APROF_(parent_CCT)(ThreadData * tdata);
void APROF_(free_CCT)(CCTNode * root);
void APROF_(print_report_CCT)(FILE * f, CCTNode * root, UInt parent_id);

/* Memory access handler (memory.c) */
VG_REGPARM(3) void APROF_(trace_access)(UWord type, Addr addr, UWord kernel_access);

/* Function entry/exit handlers (function.c) */
RoutineInfo * APROF_(new_routine_info)(ThreadData * tdata, Function * fn, UWord target);
void APROF_(destroy_routine_info)(RoutineInfo * ri);
void APROF_(function_enter)(ThreadData * tdata, Activation * act);
void APROF_(function_exit)(ThreadData * tdata, Activation * act);

/* time (time.c) */
ULong APROF_(time)(ThreadData * tdata);

#if COST == INSTR
VG_REGPARM(0) void APROF_(add_one_guest_instr)(void);
#endif

/* instrumentation events (events.c) */
void APROF_(flushEvents)(IRSB * sb);
void APROF_(addEvent_Ir)(IRSB * sb, IRExpr * iaddr, UInt isize );
void APROF_(addEvent_Dr)(IRSB * sb, IRExpr * daddr, Int dsize);
void APROF_(addEvent_Dw)(IRSB * sb, IRExpr * daddr, Int dsize);

/* Callstack management (callstack.c */
Activation * APROF_(get_activation_by_aid_drms)(ThreadData * tdata, UInt aid);
Activation * APROF_(get_activation_by_aid_rms)(ThreadData * tdata, UInt aid);

#define vgAprof_get_activation_noresize(tdata, depth) ((tdata)->stack + depth - 1)
#define vgAprof_get_activation(tdata, depth) ((depth >= tdata->max_stack_size) ? \
                                        APROF_(resize_stack)(tdata, depth) : \
                                        (tdata->stack + depth - 1))
                                        
Activation * APROF_(resize_stack)(ThreadData * tdata, UInt depth);
UInt APROF_(str_hash)(const Char *s);
void APROF_(unwind_stack)(ThreadData * tdata);
BB * APROF_(get_BB)(UWord target);
VG_REGPARM(2) void APROF_(BB_start)(UWord target, BB * bb);
Bool APROF_(trace_function)(ThreadId tid, UWord * arg, UWord * ret);

/* Debug (debug.c) */
#if DEBUG_ALLOCATION
    void APROF_(add_alloc)(UWord type);
    void APROF_(remove_alloc)(UWord type);
    void APROF_(print_alloc)(void);
    #define vgAprof_new(kind, size)     VG_(calloc)("aprof", size, 1); APROF_(add_alloc)(kind);
    #define vgAprof_delete(kind, ptr)   do { VG_(free)(prt); APROF_(remove_alloc)(kind); } while(0);
#else // DEBUG_ALLOCATION
    #define vgAprof_add_alloc(type)
    #define vgAprof_remove_alloc(type)
    #define vgAprof_new(kind, size)     VG_(calloc)("aprof", size, 1)
    #define vgAprof_delete(kind, ptr)   VG_(free)(ptr)
#endif // !DEBUG_ALLOCATION

/* main.c */
#if MEM_USAGE_INFO
void APROF_(print_info_mem_usage)(void);
#endif

/* internal debug info of valgrind */
Bool VG_(get_fnname_no_cxx_demangle) (Addr a, Char* buf, Int nbuf);

/* Syscall wrappers (syscall.c) */
void APROF_(pre_syscall)(ThreadId tid, UInt syscallno, UWord * args, 
                         UInt nArgs);
void APROF_(post_syscall)(ThreadId tid, UInt syscallno, UWord * args, 
                          UInt nArgs, SysRes res);

#define vgAprof_fix_access_size(a, s) \
                    do{ \
                        UInt diff = (a) & (APROF_(addr_multiple)-1); \
                        (a) -= diff; \
                        (s) += diff; \
                    } while(0);

#endif
