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

#define APROF_(str) VGAPPEND(vgAprof_,str)

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
#define SYSCALL_WRAPPING    1   // if 1, I/O syscall are wrapped in 
                                // order to catch external I/O

#define DEBUG_ALLOCATION    0   // if 1, check every allocation made by aprof
#define IGNORE_DL_RUNTIME   0   // if 1, disable analysis for dl_
                                // runtime_resolve (and its children)

#define REPORT_VERSION      5   // see documentation on our site: 
                                // 1 == 1.1, 2 == 1.2, ...
                                
#define DISCARD_UNKNOWN     1   // discard info about PLT or unknown 
                                // function (but this does not imply to
                                // discard info about its children)
                                
#define IGNORE_REPEAT_ACC   1   // if 1, ignore repeated accesses to 
                                // the same address within a BB
                                
#define REPORT_NAME         2   // if 1 report name is prog_TID.aprof, 
                                // if 2 is PID_TID_ADDRMULTIPLE.aprof
                                
#define CCT_GRAPHIC         0   // output CCT as dot language in an 
                                // external file (file.graph); EXPERIMENTAL

/* shadow memory  */

#define CHECK_OVERFLOW      0   // On 64bit machine, we map only 2048GB...

/*
 * We need to search an activation ts in the shadow stack...
 */
#define LINEAR              1   // Linear search (backward into stack)
#define BINARY              2   // Binary search
#define STATS               3   // Compute some stats about searching 
                                // into the stack when doing liner search
#define BACKLOG             4   // Backwarding with exponential jump, 
                                // then binary search
#define SUF2_SEARCH         LINEAR 

/* Some constants */
#define MAX_STACK_SIZE      15000   // max stack size
#define STACK_SIZE          64      // Initial stack size
#define BUFFER_SIZE         32000   // FILE buffer size
#define NAME_SIZE           4096    // function/object name buffer size 

#if defined(VG_BIGENDIAN)
#define Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
#define Endness Iend_LE
#else
#error "Unknown endianness"
#endif

/* Failure/error function */
#define AP_ASSERT(cond, msg)    do{ if (UNLIKELY(!(cond))) { \
                                    VG_(printf)("%s\n", (msg)); \
                                    tl_assert(cond); \
                                    } \
                                } while(0);

/* Data structures */

typedef IRExpr IRAtom;
// type of memory access
typedef enum access_t { LOAD, STORE, MODIFY } access_t;

#if TRACE_FUNCTION

// Used to descriminate final exit/jump of a BB
typedef enum jump_t { 
    
    BB_INIT,                            // head of BB, not used anymore
    CALL, RET, OTHER,                   // jump within a BB, not used anymore
    BBCALL, BBRET, BBOTHER,             // final exit of a BB
    NONE                                // default value
    
} jump_t;

#endif

#if DEBUG_ALLOCATION
// Used for debugging of memory usage of aprof
typedef enum alloc_type {

    BBS, RTS, FNS, 
    TS, FN_NAME, ACT,
    OBJ_NAME, POOL_PAGE, HTN,
    SEG_SUF, RMS, HT, HTNC,
    CCTS, OBJ, MANGLED,
    A_NONE

} alloc_type;
#endif

// an ELF object, instance shared by all threads
typedef struct Object {

    UWord       key;                    // Unique key for this function
    void *      next;                   // HT node attr
    char *      name;                   // Name of object
    char *      filename;               // Name of file

} Object;

// a function, instance shared by all threads
typedef struct Function {

    UWord        key;                   // unique key for this function
    void *       next;                  // HT node attr
    char *       name;                  // name of routine (demangled full)
    Object *     obj;                   // name of library the routine belongs to
    char *       mangled;               // name of routine (mangled)
    
    #if DISCARD_UNKNOWN
    Bool         discard_info;          // discard SMS/cost for this 
                                        // function (but not for its children!)
    #endif

} Function;

#if TRACE_FUNCTION

// Info about a Basic Block, instance shared by all threads 
typedef struct BB {

    UWord        key;                    // Basic block address (unique)
    void *       next;                   // HT node attr...
    UInt         instr_offset;           // length of BB (# bytes) 
    jump_t       exit;                   // jumpking of this BB
    VgSectKind   obj_section;            // ELF Object section (of the function)
    Bool         is_dl_runtime_resolve;  // Is this BB part of dl_runtime_resolve?
    Bool         is_entry;               // Is this BB first one of a function?
    Function *   fn;                     // Info about the associated function
    
} BB;

/* a code pattern is a list of tuples (start offset, length) */
struct chunk_t { int start, len; };
struct pattern {

    const char *    name;
    int             len;
    struct chunk_t  chunk[];

};

#endif

// file descriptor
typedef struct FILE {

    int        file;                     // file descriptior ID 
    char       fw_buffer[BUFFER_SIZE];   // buffer
    int        fw_pos;                   // buffer offset

} FILE;

#if CCT

// Record associated with CCT node,
typedef struct CCTNode {

    struct CCTNode *    firstChild;      // first child of the node in the CCT
    struct CCTNode *    nextSibling;     // next sibling of the node in the CCT
    ULong               routine_id;      // the routine id associated with this CCT node
    UInt                context_id;      // the context id associated with this CCT node

    #if CCT_GRAPHIC
    char *              name;            // Name of routine associated to this node
    #endif

} CCTNode;

#endif

// Info about a routine, not shared btw threads 
typedef struct {

    UWord        key;                    // Unique key for this routine
    void *       next;                   // HT node attr...
    ULong        routine_id;             // unique id for this routine
    Function *   fn;                     // Info (name, file, etc) about this routine
    Int          recursion_pending;      // number of pending activations (> 1 means recursive)
    
    #if !CCT
    HashTable *  rms_map;                // set of unique RMSInfo records for this routine
    #else
    HashTable *  context_rms_map;        // set of pairs <context_id, sms_map>
    #endif

} RoutineInfo;

// read memory size 
typedef struct {

    UWord       key;                     // rms value for this record
    void *      next;                    // HT node value
    ULong       min_cumulative_time;     // minimum time spent by the 
                                         // routine in calls with this rms
                                         
    ULong       max_cumulative_time;     // maximum time spent by the 
                                         // routine in calls with this rms
                                         
    ULong       cumulative_time_sum;     // total time spent by the 
                                         // routine in calls with this rms
    ULong       cumulative_sum_sqr;      // sum of the square of cumulative costs
    ULong       calls_number;            // number of times the routine 
                                         // has been called with this rms
    ULong       cumul_real_time_sum;     // total time spent by the routine 
                                         // in calls with this rms
                                         // without considering recursive 
                                         // call of the same function
    ULong       self_time_sum;           // total self time spent by the 
                                         // routine in calls with this rms
    ULong       self_time_min;           // minimum self time spent by 
                                         // the routine in calls with this rms
    ULong       self_time_max;           // maximum self time spent by the 
                                         // routine in calls with this rms
    ULong       self_sum_sqr;            // sum of the square of self costs

    #if INPUT_METRIC == RVMS
    ULong       rms_input_sum;            // sum of ratios RMS/RVMS
    ULong       rms_input_sum_sqr;        // sum of squares of ratios RMS/RVMS
    #endif

} RMSInfo;

// info about an activation of a routine
typedef struct {

    ULong          entry_time;           // time stamp at activation entry
    ULong          total_children_time;  // total time spent in children
    UInt           rms;                  // RMS of activation 
    
    #if INPUT_METRIC == RVMS             
    UInt           rvms;                 // RVMS of activation
    #endif
    
    RoutineInfo *  rtn_info;             // pointer to info record of 
                                         // activated routine
    UInt           aid;                  // Activation ID Activation ID 
                                         // (value of the global counter
                                         // when this act started)
    
    #if CCT
    CCTNode *      node;                 // pointer to the CCT node 
                                         // associated with the call
    #endif
    
    #if TRACE_FUNCTION
    UWord           sp;                  // Stack pointer when entered this function
    UWord           ret_addr;            // Expected BB addr of BB executed 
                                         // after a return of a called 
                                         // function (only meaningful if 
                                         // the function is called with Ijk_Call)
    Bool            skip;                // if True, disable analysis 
    #endif

} Activation;

// Info about a thread
typedef struct ThreadData {

    LookupTable *   accesses;            // stack of sets of addresses
    HashTable *     routine_hash_table;  // table of all encountered routines
    UInt            stack_depth;         // stack depth
    Activation *    stack;               // activation stack
    UInt            max_stack_size;      // max stack size
    ULong           next_routine_id;     // the routine_id that will be 
                                         // assigned to the next routine_info
    ULong           other_metric;        // needed when merging reports
    
    #if INPUT_METRIC == RMS
    UInt            next_aid;            // Activation ID that will be assigned 
                                         // to the next Activation
    #endif
    
    #if CCT
    CCTNode *       root;                // root of the CCT
    ULong           next_context_id;     // the context_id that will 
                                         // be assigned to the next CCT node
    #endif
    
    #if TIME == INSTR
    ULong            instr;              // Counter instr executed
    #elif TIME == BB_COUNT
    ULong            bb_c;               // Counter BB executed
    #elif TIME == RDTSC
    ULong            entry_time;         // Entry time for this thread
    #endif
    
        
    #if SUF2_SEARCH == STATS
    ULong            avg_depth;          // Sum of stack depth when 
                                         // doing all get_activation_by_aid
    ULong            avg_iteration;      // Sum of iterations when doing 
                                         // backwarding in all get_activation_by_aid
    ULong            ops;                // # calls of get_activation_by_aid
    #endif
    
    #if TRACE_FUNCTION
    BB *             last_bb;            // Last executed BB
    jump_t           last_exit;          // Last "final" exit/jump of last BB
    Bool             skip;               // Disable analysis
    #endif
    
    #if EVENTCOUNT
    ULong            num_func_enter;     // number of JSR events
    ULong            num_func_exit;      // number of RTS events
    ULong            num_read;           // number of memory data read operations
    ULong            num_write;          // number of memory data write operations
    ULong            num_modify;         // number of memory data read+write operations
    #endif

} ThreadData;

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
 * Merge reports of different thread of the same
 * running process. 
 * 
 * defined in main.c
 */
extern Bool APROF_(merge_report_threads);

/*
 * Merge reports of different runs of the same
 * binary program. 
 * 
 * defined in main.c
 */
extern Bool APROF_(merge_report_runs);

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
UInt APROF_(overflow_handler)(void);

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
                            Addr addr, SizeT size, Bool kernel_access);

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

/* Callstack management (callstack.c) */
Activation * APROF_(get_activation_by_aid)(ThreadData * tdata, UInt aid);

#define vgAprof_get_activation_noresize(tdata, depth) ((tdata)->stack + depth - 1)
#define vgAprof_get_activation(tdata, depth) ((depth >= tdata->max_stack_size) ? \
                                        APROF_(resize_stack)(tdata, depth) : \
                                        (tdata->stack + depth - 1))
                                        
//Activation * APROF_(get_activation)(ThreadData * tdata, unsigned int depth);
Activation * APROF_(resize_stack)(ThreadData * tdata, unsigned int depth);
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

/* internal debug info of valgrind */
Bool VG_(get_fnname_no_cxx_demangle) (Addr a, Char* buf, Int nbuf);

/* Syscall wrappers (syscall.c) */
#if INPUT_METRIC == RVMS && SYSCALL_WRAPPING == 1
void APROF_(pre_syscall)(ThreadId tid, UInt syscallno, UWord * args, 
                         UInt nArgs);
void APROF_(post_syscall)(ThreadId tid, UInt syscallno, UWord * args, 
                          UInt nArgs, SysRes res);
#endif


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
