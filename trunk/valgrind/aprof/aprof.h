/*
 * aprof global header
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
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

#define APROF_(str)         VGAPPEND(vgAprof_,str)
#define APROF_TOOL          1 // useful for data-common.h

/* Behaviour macro */
#define EMPTY_ANALYSIS      0   // if 1, analysis routines are empty 
                                // (useful in combination with EVENTCOUNT)

#define DEBUG               0   // Enable some sanity checks

#define VERBOSE             0   // 0 disabled, 1 function + thread, 2 
                                // function + thread + load/store/modify,
                                // 4 very verbose function tracing

#define EVENTCOUNT          0   // 0 disabled, 1 memory accesses, 
                                // 2 functions, 3 mem+fn
                                
#define CCT                 0   // if 1, keep a calling context tree for
                                // each thread to include context 
                                // information in reports
                                
#define COSTANT_MEM_ACCESS  1   // if 1, memory access with size >1 are 
                                //  managed as size==1
                                
                                // Performance metric:
#define NO_TIME             0   // No time 
#define INSTR               1   // Count guest intel instruction 
#define RDTSC               2   // rdtsc intel instruction timestamp
#define BB_COUNT            3   // count BB executed
#define TIME                BB_COUNT 
                            
                                 // Input estimation metric:
#define RMS                 1    // Read Memory Size
#define RVMS                2    // Read Versioned Memory Size
#define INPUT_METRIC        RVMS

#define TRACE_FUNCTION      1   // if 1, aprof traces functions by itself, 
                                // otherwise the program must be 
                                // instrumentated by GCC
                                // with -finstrument-functions
                                
#define MEM_TRACE           1   // if 0 disable mem instrumentation
#define THREAD_INPUT        1   // if 1, every write creates a new
                                // version of an input
#define SYSCALL_WRAPPING    1   // if 1, I/O syscall stores are 
                                // considered as external I/O

#define IGNORE_LOAD_SYS     0   // ignore load due to syscall

#define DEBUG_ALLOCATION    0   // if 1, check every allocation made by aprof
#define IGNORE_DL_RUNTIME   0   // if 1, disable analysis for dl_
                                // runtime_resolve (and its children)

#define REPORT_VERSION      6   // see documentation on our site: 
                                // 1 == 1.1, 2 == 1.2, ...
                                
#define DISCARD_UNKNOWN     1   // discard info about PLT or unknown 
                                // function (but this does not imply to
                                // discard info about its children)
                                
#define IGNORE_REPEAT_ACC   0   // if 1, ignore repeated accesses to 
                                // the same address within a BB
                                
#define REPORT_NAME         2   // if 1 report name is prog_TID.aprof, 
                                // if 2 is PID_TID_ADDRMULTIPLE.aprof
                                
#define CCT_GRAPHIC         0   // output CCT as dot language in an 
                                // external file (file.graph); EXPERIMENTAL

#define MEM_USAGE_INFO      0   // Check VmPeak in /proc/PID/status
                                // to get info about aprof mem usage 
/* shadow memory  */

#define CHECK_OVERFLOW      0   // On 64bit machine, we map only 2048GB...

/* Some constants */
#define MAX_STACK_SIZE      15000   // max stack size
#define STACK_SIZE          64      // Initial stack size
#define BUFFER_SIZE         32000   // FILE buffer size
#define NAME_SIZE           4096    // function/object name buffer size 

/* some config check */

#if defined(VG_BIGENDIAN)
#define Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
#define Endness Iend_LE
#else
#error "Unknown endianness"
#endif

#if INPUT_METRIC == RVMS
#define DISTINCT_RMS 1
#else
#define DISTINCT_RMS 0
#endif

#if DISTINCT_RMS
#define DEBUG_DRMS 0
#else
#define DEBUG_DRMS 0
#endif

#if THREAD_INPUT && INPUT_METRIC != RVMS
#error "THREAD_INPUT == 1 but INPUT_METRIC != RVMS"
#endif

#if SYSCALL_WRAPPING && INPUT_METRIC != RVMS
#error "SYSCALL_WRAPPING == 1 but INPUT_METRIC != RVMS"
#endif

#if INPUT_METRIC == RVMS && IGNORE_REPEAT_ACC
#error "With IGNORE_REPEAT_ACC you're ignoring a store after a load. This is not ok with RVMS"
#endif

#if INPUT_METRIC == RVMS && !THREAD_INPUT && !SYSCALL_WRAPPING
#error "RVMS need at one between THREAD_INPUT and SYSCALL_WRAPPING"
#endif

#if INPUT_METRIC == RMS
#undef DEBUG_DRMS
#endif

/* Failure/error function */
#define AP_ASSERT(cond, msg)    do{ if (UNLIKELY(!(cond))) { \
                                    VG_(printf)("%s\n", (msg)); \
                                    tl_assert(cond); \
                                    } \
                                } while(0);

/* Data structures */

#include "data-common.h"

/* Global vars */

/* Memory resolution: we can aggregate addresses in order
 * to decrese the shadow memory. 
 * - 1 => finest resolution, each byte has its timestamp
 * - 2 => every 2 byte we have a single timestamp 
 * - ...
 * defined in memory.c
 */
extern UInt APROF_(addr_multiple);

/*
 * if defined, reports will be saved in this directory
 * 
 * defined in main.c
 */
extern const HChar * APROF_(log_dir);

/* memory events (events.c) */
extern Int APROF_(events_used);

/* Info about thread currently running (thread.c) */
extern ThreadId APROF_(current_TID);
extern ThreadData * APROF_(current_tdata); 
extern UInt APROF_(running_threads); /* # running threads */

/* Basic block info HT and last BB exit type (callstack.c) */
#if TRACE_FUNCTION
extern HashTable * APROF_(bb_ht);
extern jump_t APROF_(last_exit);
#endif

/* HTs about ELF objects and functions (callstack.c) */
extern HashTable * APROF_(obj_ht);
extern HashTable * APROF_(fn_ht);

/* 
 * Global shadow memory and a global counter 
 * used for checking latest "version" of an input (memory.c)
 */
#if INPUT_METRIC == RVMS 
extern LookupTable * APROF_(global_shadow_memory);
extern UInt APROF_(global_counter);
#endif

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

#if INPUT_METRIC == RVMS
UInt APROF_(overflow_handler)(void);
#endif

#if INPUT_METRIC == RMS || DEBUG_DRMS
UInt APROF_(overflow_handler_rms)(void);
#endif

/* report functions (report.c) */
void APROF_(generate_report)(ThreadData * tdata, ThreadId tid);

/* CCT functions (CCT.c) */
#if CCT
CCTNode * APROF_(parent_CCT)(ThreadData * tdata);
void APROF_(free_CCT)(CCTNode * root);
void APROF_(print_report_CCT)(FILE * f, CCTNode * root, UInt parent_id);
#endif

/* Memory access handler (memory.c) */
VG_REGPARM(3) void APROF_(trace_access)(UWord type, 
                            Addr addr, SizeT size, UWord kernel_access);

/* Function entry/exit handlers (function.c) */
RoutineInfo * APROF_(new_routine_info)(ThreadData * tdata, Function * fn, UWord target);
void APROF_(destroy_routine_info)(RoutineInfo * ri);
void APROF_(function_enter)(ThreadData * tdata, Activation * act);
void APROF_(function_exit)(ThreadData * tdata, Activation * act);

/* time (time.c) */
#if TIME == BB_COUNT
#define vgAprof_time(tdata) (tdata)->bb_c
#else
ULong APROF_(time)(ThreadData * tdata);
#endif
#if TIME == INSTR
VG_REGPARM(0) void APROF_(add_one_guest_instr)(void);
#endif
#if TIME == BB_COUNT && !TRACE_FUNCTION
VG_REGPARM(0) void APROF_(add_one_guest_BB)(void);
#endif

/* instrumentation events (events.c) */
#if MEM_TRACE
void APROF_(flushEvents)(IRSB* sb);
void APROF_(addEvent_Ir)( IRSB* sb, IRAtom* iaddr, UInt isize );
void APROF_(addEvent_Dr)(IRSB* sb, IRAtom* daddr, Int dsize);
void APROF_(addEvent_Dw)(IRSB* sb, IRAtom* daddr, Int dsize);
#endif

/* Callstack management (callstack.c */
#if INPUT_METRIC == RVMS
Activation * APROF_(get_activation_by_aid_rvms)(ThreadData * tdata, UInt aid);
#endif
#if INPUT_METRIC == RMS || DEBUG_DRMS
Activation * APROF_(get_activation_by_aid_rms)(ThreadData * tdata, UInt aid);
#endif

#define vgAprof_get_activation_noresize(tdata, depth) ((tdata)->stack + depth - 1)
#define vgAprof_get_activation(tdata, depth) ((depth >= tdata->max_stack_size) ? \
                                        APROF_(resize_stack)(tdata, depth) : \
                                        (tdata->stack + depth - 1))
                                        
//Activation * APROF_(get_activation)(ThreadData * tdata, unsigned int depth);
Activation * APROF_(resize_stack)(ThreadData * tdata, UInt depth);
UInt APROF_(str_hash)(const Char *s);

#if TRACE_FUNCTION
void APROF_(unwind_stack)(ThreadData * tdata);
BB * APROF_(get_BB)(UWord target);
VG_REGPARM(2) void APROF_(BB_start)(UWord target, BB * bb);
#else
Bool APROF_(trace_function)(ThreadId tid, UWord * arg, UWord * ret);
#endif

/* Debug (debug.c) */
#if DEBUG_ALLOCATION
void APROF_(add_alloc)(UWord type);
void APROF_(remove_alloc)(UWord type);
void APROF_(print_alloc)(void);
#endif

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

#if CCT_GRAPHIC
void APROF_(print_cct_graph)(FILE * f, CCTNode* root, UInt parent_id, 
                               char * parent_name);
#endif

#endif
