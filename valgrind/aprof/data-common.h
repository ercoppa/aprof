#ifndef APROF_DATA_H
#define APROF_DATA_H

#if APROF_TOOL

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

#endif // TRACE_FUNCTION

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
#endif // DEBUG_ALLOCATION

#endif // APROF_TOOL

// an ELF object, instance shared by all threads
typedef struct Object {

    UWord       key;                    // Unique key for this function
    void *      next;                   // HT node attr
    HChar *     name;                   // Name of object
    HChar *     filename;               // Name of file

} Object;

// a function, instance shared by all threads
typedef struct Function {

    UWord        key;                   // unique key for this function
    void *       next;                  // HT node attr
    HChar *      name;                  // name of routine (demangled full)
    Object *     obj;                   // name of library the routine belongs to
    HChar *      mangled;               // name of routine (mangled)
    
    #if DISCARD_UNKNOWN
    Bool         discard_info;          // discard SMS/cost for this 
                                        // function (but not for its children!)
    #endif

} Function;

#if APROF_TOOL
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

#endif // TRACE_FUNCTION

// file descriptor
typedef struct FILE {

    Int        file;                     // file descriptior ID 
    HChar      fw_buffer[BUFFER_SIZE];   // buffer
    Int        fw_pos;                   // buffer offset

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

#endif // CCT

#endif // APROF_TOOL

typedef struct {
    
    UWord         key;
    void *        next;
    ULong         calls;
    
} RMSValue;

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
    
    #if DISTINCT_RMS
    HashTable * distinct_rms;             // ht of RMS seen
    #endif
    
    #if EXTERNAL
    ULong       total_calls;
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
    #if EXTERNAL
    double       cumulative_sum_sqr;      // sum of the square of cumulative costs
    #else
    ULong       cumulative_sum_sqr;      // sum of the square of cumulative costs
    #endif
    
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
    #if EXTERNAL                                     
    double      self_sum_sqr;            // sum of the square of self costs
    #else
    ULong       self_sum_sqr;            // sum of the square of self costs
    #endif

    #if INPUT_METRIC == RVMS
    ULong       rms_input_sum;            // sum of RMS
    #if EXTERNAL
    double       rms_input_sum_sqr;        // sum of squares of RMS
    #else
    ULong       rms_input_sum_sqr;        // sum of squares of RMS
    #endif
    #endif

} RMSInfo;

#if APROF_TOOL

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
    
    #if IGNORE_DL_RUNTIME
    Bool            skip;                // if True, disable analysis 
    #endif
    
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
    
    #if IGNORE_DL_RUNTIME
    Bool             skip;               // Disable analysis
    #endif
    
    #endif
    
    #if EVENTCOUNT
    ULong            num_func_enter;     // number of JSR events
    ULong            num_func_exit;      // number of RTS events
    ULong            num_read;           // number of memory data read operations
    ULong            num_write;          // number of memory data write operations
    ULong            num_modify;         // number of memory data read+write operations
    #endif

} ThreadData;

#endif // APROF_TOOL

#endif // APROF_DATA_H
