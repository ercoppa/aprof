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
#include "pub_tool_xarray.h"
#include "valgrind.h"
#include "pub_tool_mallocfree.h"
#include "hashtable/hashtable.h"
#include "../coregrind/pub_core_options.h"
#include "pool/pool.h"

/* Behaviour macro */
#define SUF					2	// Implementation of stack union find {1,2}
#define CHECK_SUF_OVERFLOW	0	// On 64bit machine, we map only 16GB...
#define EMPTY_ANALYSIS		0	// if 1, analyesis routines are empty (use in combination with EVENTCOUNT)
#define DEBUG				0	// Enable some sanity checks
#define VERBOSE				0	// 0 disabled, 1 function + thread, 2 function + thread + load/store/modify
#define EVENTCOUNT			0	// 0 disabled, 1 memory accesses, 2 functions, 3 mem+fn
#define CCT					0	// if 1, keep a calling context tree for each thread to include context information in reports
#define ADDR_MULTIPLE		4	// account only accessed address muliple of this number, min 1
#define COSTANT_MEM_ACCESS	1	// if 1, memory access with size >1 are managed as size==1
#define NO_TIME				0	// No time 
#define INSTR				1	// Count guest intel instruction 
#define RDTSC				2	// rdtsc intel instruction timestamp
#define BB_COUNT			3	// count BB executed
#define TIME				BB_COUNT

#if SUF == 2
#define LINEAR				1	// Linear search (backward into stack)
#define BINARY				2	// Binary search
#define STATS				3	// Compute some stats about searching into the stack when doing liner search
#define SUF2_SEARCH			LINEAR
#endif

#define TRACE_FUNCTION		1	// if 1, aprof trace functions by itself, otherwise it supposes
								// that the program is instrumentated by GCC
								// with -finstrument-functions
#define MEM_TRACE			1	// if 0 disable mem instrumentation
#define CCT_GRAPHIC			0	// if 1, create a report compatible with graphviz about CCT tree
#define DEBUG_ALLOCATION	0	// if 1, check every allocation maded by aprof
#define IGNORE_DL_RUNTIME	1	// if 1, disable analysis for dl_runtime_resolve (and its children)
#define REPORT_VERSION		1	// see documentation on  our site
#define DISCARD_UNKNOWN		1	// discard info about PLT or unknown function (but this not implies to discard info about its children)

/* Some constants */
#define STACK_SIZE		64		// Initial stack size
#define BUFFER_SIZE		32000	// FILE buffer size
#define NAME_SIZE		4096	// function/object name buffer size 

#if defined(VG_BIGENDIAN)
#define Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
#define Endness Iend_LE
#else
#error "Unknown endianness"
#endif

#include "SUF/union_find.h"
#include "SUF2/suf.h"

/* Data type */

typedef IRExpr IRAtom;
// type of memory access
typedef enum access_t { LOAD, STORE, MODIFY } access_t;

#if TRACE_FUNCTION

// Used to descriminate final exit/jump of a BB
typedef enum jump_t { 
	BB_INIT,							// head of BB, not used anymore
	CALL, RET, OTHER,					// jump within a BB, not used anymore
	BBCALL, BBRET, BBOTHER,				// final exit of a BB
	NONE								// default value
} jump_t;

#endif

#if DEBUG_ALLOCATION
// Used for debugging of memory usage of aprof
typedef enum alloc_type {
	BBS, RTS, FNS, 
	TS, FN_NAME, ACT,
	OBJ_NAME, POOL_PAGE, HTN,
	SEG_SUF, SMS, HT,
	CCTS, OBJ, MANGLED,
	A_NONE
} alloc_type;
#endif

/* Data structures */

// Common/global to all threads
typedef struct Object {
	UWord		key;					// Unique key for this function
	void *		next;					// HT node attr
	char *		name;					// Name of object
	char *		filename;				// Name of file
} Object;

// Common/global to all threads
typedef struct Function {
	UWord		key;					// unique key for this function
	void *		next;					// HT node attr
	char *		name;					// name of routine (demangled full)
	Object *	obj;					// name of library the routine belongs to
	char *		mangled;				// name of routine (mangled)
	#if DISCARD_UNKNOWN
	Bool		discard_info;			// discard SMS/cost for this function (but not for its children!)
	#endif
} Function;

#if TRACE_FUNCTION

// Info about a Basic Block, common/global to all threads 
typedef struct BB {

	UWord		key;					// Basic block address (unique)
	void *		next;					// HT node attr...
	UInt		instr_offset;			// length of BB (# bytes) 
	jump_t		exit;					// jumpking of this BB
	VgSectKind	obj_section;			// ELF Object section (of the function)
	Bool		is_dl_runtime_resolve;	// Is this BB part of dl_runtime_resolve?
	Bool		is_entry;				// Is this BB first one of a function?
	Function *	fn;						// Info about the associated function
	
} BB;

/* a code pattern is a list of tuples (start offset, length) */
struct chunk_t { int start, len; };
struct pattern {
	const char *	name;
	int				len;
	struct chunk_t	chunk[];
};

#endif

typedef struct FILE {
	int		file;						// file descriptior ID 
	char	fw_buffer[BUFFER_SIZE];		// buffer
	int		fw_pos;						// buffer offset
} FILE;

#if CCT
// Record associated with CCT node,
typedef struct CCTNode {
	struct CCTNode *	firstChild;		// first child of the node in the CCT
	struct CCTNode *	nextSibling;	// next sibling of the node in the CCT
	ULong				routine_id;		// the routine id associated with this CCT node
	UInt				context_id;		// the context id associated with this CCT node
	#if CCT_GRAPHIC
	char * 				name;			// Name of routine assiated to this node
	#endif
} CCTNode;
#endif

// Info about routine, this is thread specific! 
typedef struct {
	UWord		key;					// Unique key for this routine
	void *		next;					// HT node attr...
	ULong		routine_id;				// unique id for this routine
	Function *	fn;						// Info (name, file, etc) about this routine
	ULong	 	total_self_time;		// total self time for this routine
	ULong		total_cumulative_time;	// printf("curr_depth = %d\n", curr_depth); total cumulative time for this routine
	UInt		calls;					// number times this routine has been called
	Int			recursive;				// 1 if the routine has ever been called recursively
	Int			recursion_pending;		// number of pending activations (> 1 means recursive)
	#if !CCT
	HashTable *	sms_map;				// set of unique SMSInfo records for this routine
	#else
	HashTable *	context_sms_map;		// set of pairs <context_id, sms_map>
	#endif
} RoutineInfo;

typedef struct {
	UWord	key;						// sms value for this record
	void *	next;						// HT node value
	UInt	max_cumulative_time;		// maximum time spent by the routine in calls with this sms
	UInt	min_cumulative_time;		// minimum time spent by the routine in calls with this sms
	ULong	cumulative_time_sum;		// total time spent by the routine in calls with this sms
	ULong	calls_number;				// number of times the routine has been called with this sms
	ULong	cumulative_time_sqr_sum;	// sum of squares of times spent by the routine in calls with this sms
} SMSInfo;

typedef struct {
	ULong			entry_time;			// time stamp at activation entry
	ULong			total_children_time;// total time spent in children
	UInt			sms;				// SMS of activation (always positive when an activation ends)
	RoutineInfo *	rtn_info;			// pointer to info record of activated routine
	#if CCT
	CCTNode *		node;				// pointer to the CCT node associated with the call
	#endif
	#if SUF == 2
	UInt			aid;				// Activation ID
	UInt			old_aid;			// Activation ID of the caller
	#endif
	#if TRACE_FUNCTION
	UWord			sp;					// Stack pointer when entered this function
	UWord			ret_addr;			// Expected BB addr of BB executed after a return of a called function (only meaningful if the function is called with Ijk_Call)
	Bool			skip;				// if True, disable analysis 
	#endif
} Activation;

typedef struct ThreadData {
	#if SUF == 1
	UnionFind *		accesses;			// stack of sets of addresses
	#elif SUF == 2
	StackUF *		accesses;			// stack of sets of addresses
	#endif
	HashTable *		routine_hash_table;	// table of all encountered routines
	int				stack_depth;		// stack depth
	Activation *	stack;				// activation stack
	UInt			max_stack_size;		// max stack size
	ULong			next_routine_id;	// the routine_id that will be assigned to the next routine_info
	#if CCT
	CCTNode *		root;				// root of the CCT
	ULong			next_context_id;	// the context_id that will be assigned to the next CCT node
	#endif
	#if TIME == INSTR
	ULong			instr;				// Counter instr executed
	#elif TIME == BB_COUNT
	ULong			bb_c;				// Counter BB executed
	#endif
	#if SUF == 2
	UInt			next_aid;			// Activation aid that will be assigned to the next Activation
	UInt			curr_aid;			// Current Activation aid (runtime)
	#if SUF2_SEARCH == STATS
	ULong			avg_depth;			// Sum of stack depth when doing all get_activation_by_aid
	ULong			avg_iteration;		// Sum of iterations when doing backwarding in all get_activation_by_aid
	ULong			ops;				// # calls of get_activation_by_aid
	#endif
	#endif
	#if TRACE_FUNCTION
	BB *			last_bb;			// Last executed BB
	jump_t			last_exit;			// Last "final" exit/jump of last BB
	Bool			skip;				// Disable analysis
	#endif
	#if EVENTCOUNT
	ULong			num_func_enter;		// number of JSR events
	ULong			num_func_exit;		// number of RTS events
	ULong			num_read;			// number of memory data read operations
	ULong			num_write;			// number of memory data write operations
	ULong			num_modify;			// number of memory data read+write operations
	#endif
} ThreadData;

/* Global vars */

// Info about thread currently running
extern ThreadId current_TID;
extern ThreadData * current_tdata; 

#if TRACE_FUNCTION
extern HashTable * bb_ht;
extern jump_t last_exit;
#endif

extern HashTable * obj_ht;
extern HashTable * fn_ht;

/* Failure/error function */
#define AP_ASSERT(cond, msg)	{ if (!(cond)) { \
									VG_(printf)("%s\n", (msg)); \
									tl_assert(cond); \
									} \
								}

/* file functions */
FILE * ap_fopen(char * name);
void ap_fflush(FILE * f);
void ap_fwrite(FILE * f, char * buffer, unsigned int size);
void ap_fclose(FILE * f);

/* thread functions */
void switch_thread(ThreadId tid, ULong blocks_dispatched);
void thread_exit (ThreadId tid);

/* report functions */
void generate_report(ThreadData * tdata, ThreadId tid);

/* CCT functions */
#if CCT
CCTNode * parent_CCT(ThreadData * tdata);
void freeTree(CCTNode * root);
void print_cct_info(FILE * f, CCTNode * root, UInt parent_id);

#if CCT_GRAPHIC
void print_cct_graph(FILE * f, CCTNode* root, UInt parent_id, char * parent_name);
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
ULong ap_time(void);
#if TIME == INSTR
VG_REGPARM(0) void add_one_guest_instr(void);
#endif
#if TIME == BB_COUNT && !TRACE_FUNCTION
VG_REGPARM(0) void add_one_guest_BB(void);
#endif

/* events */
#if MEM_TRACE
void flushEvents(IRSB* sb);
void addEvent_Ir ( IRSB* sb, IRAtom* iaddr, UInt isize );
void addEvent_Dr (IRSB* sb, IRAtom* daddr, Int dsize);
void addEvent_Dw (IRSB* sb, IRAtom* daddr, Int dsize);
#endif

/* Callstack management */
#if SUF == 2
Activation * get_activation_by_aid(ThreadData * tdata, UInt aid);
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
void remove_alloc(UWord type);
void print_alloc(void);
#endif

/* internal debug info of valgrind */
Bool VG_(get_fnname_no_cxx_demangle) (Addr a, Char* buf, Int nbuf);

#endif
