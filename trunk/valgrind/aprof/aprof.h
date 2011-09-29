/*
 * Aprof global header
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
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
#include "valgrind.h"
#include "pub_tool_mallocfree.h"
#include "hashtable/hashtable.h"
#include "pool/pool.h"

/* Behaviour macro */
#define SUF					2	// Implementation of stack union find {1,2}
#define EMPTY_ANALYSIS		0	// if 1, analyesis routines are empty (performance benchmark reference)
#define TRACER 				0	// Create a trace for testing SUF
#define DEBUG				1	// Enable some sanity checks
#define VERBOSE				0	// 0 disabled, 1 function + thread, 2 function + thread + load/store/modify, 5 elaborated functions
#define EVENTCOUNT			0	// 0 disabled, 1 memory accesses, 2 function entries/exits, 3 mem+fn, 4 mem+fn+thread
#define CCT					0	// if 1, keep a calling context tree for each thread to include context information in reports
#define ADDR_MULTIPLE		4	// account only accessed address muliple of this number, min 1
#define COSTANT_MEM_ACCESS	1	// if 1, memory access with size >1 are managed as size==1
#define NO_TIME				0	// No time 
#define INSTR				1	// Count guest intel instruction 
#define RDTSC				2	// rdtsc intel instruction
#define BB_COUNT			3	// count BB executed
#define TIME				BB_COUNT
#define LINEAR				1	// Linear search (backward into stack)
#define BINARY				2	// Binary search
#define STATS				3	// Compute some stats about searching into the stack when doing liner search
#define SUF2_SEARCH			LINEAR
#define TRACE_FUNCTION		1	// if 1, aprof trace functions by itself, otherwise it suppose 
								// that the program is instrumentated by GCC
								// with -finstrument-functions
#define MEM_TRACE			1	// if 0 disable memory tracing
#define CCT_GRAPHIC			0	// if 1, create a report that draw the CCT tree with graphviz 
#define DEBUG_ALLOCATION	0	// if 1, check every allocation maded by aprof

#if SUF == 1
#include "SUF/union_find.h"
#elif SUF == 2
#include "SUF2/suf.h"
#endif

/* Some constants */
#define STACK_SIZE   64
#define BUFFER_SIZE  32000
#define FN_NAME_SIZE 256
#define POOL_ELEM 1024

#if defined(VG_BIGENDIAN)
# define Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
# define Endness Iend_LE
#else
# error "Unknown endianness"
#endif

/* Data type */
#ifndef UWord64
#define UWord64 unsigned long long 
#endif
typedef IRExpr IRAtom;
typedef enum access_t { LOAD, STORE, MODIFY } access_t;

#if TRACE_FUNCTION

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

#endif

#if DEBUG_ALLOCATION
typedef enum alloc_type {
	BBS, RTS, FNS, 
	TS, FN_NAME, ACT,
	OBJ_NAME, POOL_PAGE, HTN,
	SEG_SUF, SMS, HT,
	CCTS,
	A_NONE
} alloc_type;
#endif

/* Data structures */

typedef struct Object {
	UWord		  key;
	void 		* next;
	char 		* name;
	char		* filename;
} Object;

typedef struct Function {
	UWord		  key;
	void 		* next;
	char		* name;					// name of routine
	Object		* obj;					// name of library the routine belongs to
} Function;

#if TRACE_FUNCTION

/* Info about a Basic Block */
typedef struct BB {

	/* Basic block address */
	UWord key;
	void * next;
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
	/* Object section (of the function) */
	VgSectKind obj_section;
	/* Is this BB part of dl_runtime_resolve? */
	Bool is_dl_runtime_resolve;
	/* Is this BB first one of a function? */
	Bool is_entry;
	/* Info about the associated function */
	Function * fn;
	
} BB;

/* a code pattern is a list of tuples (start offset, length) */
struct chunk_t { int start, len; };
struct pattern
{
    const char* name;
    int len;
    struct chunk_t chunk[];
};

#endif

typedef struct FILE {
	int file;
	char fw_buffer[BUFFER_SIZE];
	int fw_pos;
} FILE;

#if CCT
// Record associated with CCT node
typedef struct CCTNode {
	struct CCTNode * firstChild;		// first child of the node in the CCT
	struct CCTNode * nextSibling;		// next sibling of the node in the CCT
	UWord64			 routine_id;		// the routine id associated with this CCT node
	UWord64			 context_id;		// the context id associated with this CCT node
	#if CCT_GRAPHIC
	char * name;
	#endif
} CCTNode;
#endif

typedef struct {
	UWord		  key;
	void 		* next;
	UWord64 routine_id;					// unique id for this routine
	Function * fn;						// Info (name, file, etc) about this routine
	UWord64 total_self_time;			// total self time for this routine
	UWord64 total_cumulative_time;		// printf("curr_depth = %d\n", curr_depth); total cumulative time for this routine
	UWord calls;						// number times this routine has been called
	UWord recursive;					// 1 if the routine has ever been called recursively
	UWord recursion_pending;			// number of pending activations (> 1 means recursive)
	#if !CCT
	HashTable * sms_map;				// set of unique SMSInfo records for this routine
	#else
	HashTable * context_sms_map;		// set of pairs <context_id, sms_map>
	#endif
} RoutineInfo;

typedef struct {
	UWord   key;						// sms value for this record
	void  * next;
	UWord64 max_cumulative_time;		// maximum time spent by the routine in calls with this sms
	UWord64 min_cumulative_time;		// minimum time spent by the routine in calls with this sms
	UWord64 partial_cumulative_time;	// total time spent by the routine in calls with this sms
	UWord partial_calls_number;			// number of times the routine has been called with this sms
} SMSInfo;

typedef struct {
	UWord64 entry_time;					// time stamp at activation entry
	UWord64 total_children_time;		// total time spent in children
	UWord sms;							// SMS of activation (always positive when an activation ends)
	RoutineInfo * rtn_info;				// pointer to info record of activated routine
	#if CCT
	CCTNode * node;						// pointer to the CCT node associated with the call
	#endif
	#if SUF == 2
	UWord aid;
	UWord old_aid;
	#endif
	#if TRACE_FUNCTION
	UWord sp;							// Stack pointer when entered this function
	/* 
	 * If A() call  B() (and BB of A has as jumpkind Ijk_Call),
	 * when B() returned, I will expect to execute the BB of A() 
	 * with this address.
	 */
	UWord ret_addr;
	Bool skip;						// if True, do not trace
	#endif
} Activation;

typedef struct {
	#if SUF == 1
	UnionFind * accesses;				// stack of sets of addresses
	#elif SUF == 2
	StackUF * accesses;					// stack of sets of addresses
	#endif
	HashTable * routine_hash_table;		// table of all encountered routines
	int stack_depth;					// stack depth
	Activation * stack;					// activation stack
	UWord max_stack_size;				// max stack size
	UWord64 next_routine_id;			// the routine_id that will be assigned to the next routine_info
	#if CCT
	CCTNode * root;						// root of the CCT
	UWord64 next_context_id;				// the context_id that will be assigned to the next CCT node
	#endif
	#if TIME == INSTR
	UWord64 instr;
	#elif TIME == BB_COUNT
	UWord bb_c;
	#endif
	#if TRACER
	FILE * log;
	#endif
	#if SUF == 2
	UWord next_aid;
	UWord curr_aid;
	#endif
	#if TRACE_FUNCTION
	BB * last_bb;
	jump_t last_exit;
	Bool skip;
	#endif
} ThreadData;

/* Global vars */

extern ThreadId current_TID;
extern ThreadData * current_tdata; 
extern pool_t * rtn_pool;
extern void * rtn_free_list;
extern pool_t * sms_pool;
extern void * sms_free_list;

#if TRACE_FUNCTION
extern HashTable * bb_ht;
extern pool_t * bb_pool;
extern void * bb_free_list;
#endif

extern pool_t * obj_pool;
extern void * obj_free_list;
extern HashTable * obj_ht;
extern pool_t * fn_pool;
extern void * fn_free_list;
extern HashTable * fn_ht;

/* Counter */ 
#if EVENTCOUNT
extern UWord read_n;
extern UWord write_n;
extern UWord modify_n;
extern UWord fn_in_n;
extern UWord fn_out_n;
extern UWord thread_n;
extern UWord bb_c;
#endif

#if SUF2_SEARCH == STATS
extern UWord64 avg_depth;
extern UWord64 avg_iteration;
extern UWord64 ops;
#endif

#if TRACE_FUNCTION
extern jump_t last_exit;
#endif

/* Failure/error function */
#define AP_ASSERT(cond, msg)	{ if (!(cond)) { \
									VG_(printf)("%s\n", (msg)); \
									tl_assert(cond); \
									} \
								}
void failure(char * msg); /* Deprecated */

/* file functions */
FILE * ap_fopen(char * name);
void ap_fflush(FILE * f);
void ap_fwrite(FILE * f, char * buffer, unsigned int size);
void ap_fclose(FILE * f);

/* thread functions */
void switch_thread(ThreadId tid, ULong blocks_dispatched);
void thread_exit (ThreadId tid);
ThreadId thread_running (void);
ThreadData * get_thread_data(ThreadId tid);	// if tid == 0, we ask to Valgrind 

/* report functions */
void generate_report(ThreadData * tdata, ThreadId tid);

/* CCT functions */
#if CCT
CCTNode * parent_CCT(ThreadData * tdata);
void freeTree(CCTNode * root);
void print_cct_info(FILE * f, CCTNode * root, UWord parent_id);

#if CCT_GRAPHIC
void print_cct_graph(FILE * f, CCTNode* root, UWord parent_id, char * parent_name);
#endif

#endif

/* Memory access  handler */
VG_REGPARM(3) void trace_access(UWord type, Addr addr, SizeT size);

/* Function entry/exit handler */
RoutineInfo * new_routine_info(ThreadData * tdata, Function * fn, UWord target);
void destroy_routine_info(void * data);
void function_enter(ThreadData * tdata, Activation * act);
void function_exit(ThreadData * tdata, Activation * act);

/* time */
UWord64 ap_time(void);
#if TIME == INSTR
VG_REGPARM(0) void add_one_guest_instr(void);
#endif
#if TIME == BB_COUNT && !TRACE_FUNCTION
VG_REGPARM(0) void add_one_guest_BB(void);
#endif

/* events */
void flushEvents(IRSB* sb);
void addEvent_Ir ( IRSB* sb, IRAtom* iaddr, UInt isize );
void addEvent_Dr (IRSB* sb, IRAtom* daddr, Int dsize);
void addEvent_Dw (IRSB* sb, IRAtom* daddr, Int dsize);

/* Callstack management */
#if SUF == 2
Activation * get_activation_by_aid(ThreadData * tdata, UWord aid);
#endif

#define get_activation(tdata, depth) ((depth-1 >= tdata->max_stack_size) ? \
										resize_stack(tdata, depth) : \
										(tdata->stack + depth - 1))
//Activation * get_activation(ThreadData * tdata, unsigned int depth);
Activation * resize_stack(ThreadData * tdata, unsigned int depth);

#if TRACE_FUNCTION
BB * get_BB(UWord target);
VG_REGPARM(2) void BB_start(UWord target, BB * bb);
#else
Bool trace_function(ThreadId tid, UWord * arg, UWord * ret);
#endif

/* Debug */
#if DEBUG_ALLOCATION
void add_alloc(UWord type);
void print_alloc(void);
#endif

#endif
