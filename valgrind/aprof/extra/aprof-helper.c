/*
 * aprof-helper
 */

/*
 * Some conf params for data structures (used also by aprof)
 * in data-common.h
 */
#define DISTINCT_RMS        1
#define REPORT_VERSION      6
#define REPORT_VERSION_OLD  4
                                 // Input estimation metric:
#define RMS                 1    // Read Memory Size
#define RVMS                2    // Read Versioned Memory Size
#define INPUT_METRIC        RVMS

#define INPUT_STATS         1

#define EXTERNAL 1

#include <stdio.h>
#include "valgrind-types.h"
#include "../hashtable/hashtable.h"
#include "../data-common.h"
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEBUG 1

#define DELIM_SQ '$'
#define DELIM_DQ "$"

#define EMSG(...) fprintf(stderr, __VA_ARGS__);

#define BG_GREEN   "\x1b[42m"
#define BG_YELLOW  "\x1b[43m"
#define BG_RESET   "\x1b[0m"
#define BG_RED     "\x1b[41m"

#define YELLOW(str) BG_YELLOW str BG_RESET
#define GREEN(str) BG_GREEN str BG_RESET
#define RED(str) BG_RED str BG_RESET

#define ASSERT(cond, ...)   do { \
                                    if (!(cond)) { \
                                        EMSG(RED("Fatal error:")" "); \
                                        EMSG(__VA_ARGS__); \
                                        EMSG("\n"); \
                                        assert(cond); \
                                    } \
                            } while(0);

// check for overflow (long)
#define ADD(dest, inc) do { \
                            ULong old = dest; \
                            dest += inc; \
                            ASSERT(dest >= old, "overflow"); \
                            } while(0); 

// double
#define ADD_D(dest, inc) do { \
                            double old = dest; \
                            dest += inc; \
                            ASSERT(dest >= old, "overflow"); \
                            } while(0);

#define MAX(a,b) do { if (b > a) a = b; } while(0);
#define ABS_DIFF(val,a,b) do { val = a - b; if (val < 0) val = -val; } while(0);

// check under/overflow strtol
#define UOF_LONG(val, line) ASSERT(errno != ERANGE, \
                            "under/over flow: %s", line);

// check under/overflow strtod
#define UOF_DOUBLE(val, line) ASSERT(errno != ERANGE, \
                            "under/over flow: %s", line);

// Debug assert
#if DEBUG
#define DASSERT(cond, ...) ASSERT(cond, __VA_ARGS__)
#else // DEBUG
#define DASSERT(cond, ...) // do nothing
#endif // DEBUG

#define STR(buf, ...) sprintf(buf, __VA_ARGS__);

#define SLOT 2

// compare modes:
#define LOOSE           0
#define STRICT          1
#define TOLLERANCE      2

// assert if mode is STRICT
#define SASSERT(...) if(mode == STRICT){ASSERT(__VA_ARGS__)};
#define IFS(cond) if (mode == STRICT && cond)
// assert if mode is TOLLERANCE
#define TASSERT(...) if(mode == TOLLERANCE){ASSERT(__VA_ARGS__)};
#define IFT(cond) if (mode == TOLLERANCE && cond)
// assert if mode is not LOOSE
#define TSASSERT(...) if(mode != LOOSE){ASSERT(__VA_ARGS__)};

#define INPUT_TOLLERANCE_P 5   // percentage
#define INPUT_TOLLERANCE   3   // abs value
#define COST_TOLLERANCE_P  15  // percentage
#define COST_TOLLERANCE    150 // abs value

#define STR_ALIGN 48
#define NUM_ALIGN 8

#define DIR_MERGE_THREAD "merge_by_pid"
#define DIR_MERGE_RUNS   "merge_by_cmd"
#define DIR_MERGE_BOTH   "merge_by_pid_cmd"
#define DIR_MERGE_ALL    "merge_all"


static Bool consistency = False;
static Bool compare = False;
static Bool merge_all = False;
static Bool merge_runs = False;
static Bool merge_threads = False;
static HChar * directory = NULL;
static HChar * logs[SLOT] = {NULL, NULL}; // only for compare
static HChar * rtn_skip[] = { "madvise" };

typedef struct aprof_report {
    
    UInt version;
    UInt input_metric;
    UInt memory_resolution;
    HChar * cmd;
    HChar * app;
    
    ULong performance_metric;
    ULong sum_distinct_rms;
    ULong real_total_cost;
    ULong self_total_cost;
    
    HashTable * fn_ht;
    HashTable * obj_ht;
    HashTable * routine_hash_table;

    ULong next_routine_id;
    
    ULong tmp;
    Bool sqr_over;
    Bool self_sqr_over;

} aprof_report;

static aprof_report ap_rep[SLOT];

static void fn_destroy(void * fnt) {
    Function * fn = (Function *) fnt;
    VG_(free)(fn->name);
    VG_(free)(fn->mangled);
    VG_(free)(fnt);
}

static void obj_destroy(void * obj) {
    Object * o = (Object *) obj;
    VG_(free)(o->name);
    VG_(free)(o);
}

static void destroy_routine_info(void * rtn) {
    
    RoutineInfo * ri = (RoutineInfo *) rtn;
    HT_destruct(ri->rvms_map);
    HT_destruct(ri->distinct_rms);
    
    VG_(free)(rtn);
}

static UInt get_memory_resolution_report(HChar * report) {
    
    HChar * rep = basename(report);
    ASSERT(rep != NULL && strlen(rep) > 0, "Invalid report");
    
    // report: PID_TID_RES[_N].aprof
    
    // skip PID_
    UInt pos = 0;
    while (rep[pos] != '_' && rep[pos] != '\0') pos++;
    ASSERT(pos > 0, "Invalid report name: %s", report);
    
    // skip TID_
    UInt t_pos = pos + 1;
    while (rep[t_pos] != '_' && rep[t_pos] != '\0') t_pos++;
    ASSERT(t_pos > pos + 1, "Invalid report name: %s", report);
    
    UInt r_pos = t_pos + 1;
    while (rep[r_pos] != '.' && rep[r_pos] != '_' && rep[t_pos] != '\0') r_pos++;
    ASSERT(r_pos > t_pos + 1, "Invalid report name: %s", report);
    
    HChar res_s[16];
    strncpy(res_s, rep + t_pos + 1, r_pos - t_pos); 
    UInt res = strtol(res_s, NULL, 10);
    
    return res;
}

static UInt get_tid_report(HChar * report) {
    
    HChar * rep = basename(report);
    ASSERT(rep != NULL && strlen(rep) > 0, "Invalid report");
    
    // report: PID_TID_RES[_N].aprof
    
    // skip PID_
    UInt pos = 0;
    while (rep[pos] != '_' && rep[pos] != '\0') pos++;
    ASSERT(pos > 0, "Invalid report name: %s", report);
    
    UInt t_pos = pos + 1;
    while (rep[t_pos] != '_' && rep[t_pos] != '\0') t_pos++;
    ASSERT(t_pos > pos + 1, "Invalid report name: %s", report);
    
    HChar tid_s[16];
    strncpy(tid_s, rep + pos + 1, t_pos - pos);  
    UInt tid = strtol(tid_s, NULL, 10);

    //printf("Tid = %u report %s\n", tid, report);
    return tid;
}

static UInt get_pid_report(HChar * report) {
    
    HChar * rep = basename(report);
    ASSERT(rep != NULL && strlen(rep) > 0, "Invalid report");
    
    UInt pos = 0;
    while (rep[pos] != '_' && rep[pos] != '\0') pos++;
    ASSERT(pos > 0, "Invalid report name: %s", report);
    
    HChar * pid_s = strndup(rep, pos); 
    UInt pid = strtol(pid_s, NULL, 10);
    
    free(pid_s);
    return pid;
}

static RoutineInfo * new_routine_info(Function * fn, UWord target, 
                                        aprof_report * r) {
    
    DASSERT(fn != NULL, "Invalid function info");
    DASSERT(target > 0, "Invalid target");
    
    RoutineInfo * rtn_info = VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
    DASSERT(rtn_info != NULL, "rtn info not allocable");
    
    rtn_info->key = target;
    rtn_info->fn = fn;
    rtn_info->routine_id = r->next_routine_id++;
    rtn_info->distinct_rms = 0;

    /* elements of this ht are freed when we generate the report */
    rtn_info->rvms_map = HT_construct(free);
    DASSERT(rtn_info->rvms_map != NULL, "rvms_map not allocable");
    
    rtn_info->distinct_rms = HT_construct(free);
    DASSERT(rtn_info->distinct_rms != NULL, "rvms_map not allocable");
    
    HT_add_node(r->routine_hash_table, rtn_info->key, rtn_info);
    
    return rtn_info;
}

static HChar * put_delim(HChar * str_orig) {
    
    HChar * str;
    Int size = strlen(str_orig);
    DASSERT(size > 0, "Invalid size of input string");
    
    str = malloc(size + 1);
    str = strcpy(str, str_orig);
    
    Int skip = 0;
    Int i = 0;
    for (i = 0; i < size; i++) {
        
        if (str[i] == ' ' && !skip)
            str[i] = DELIM_SQ;
        
        else if (str[i] == '"')
            skip = ~skip;
        
        else if (str[i] == '\0')
            return str;
    }
    
    return str;
}

static UInt str_hash(const HChar * str) {
    
    const HChar * s = str;
    UInt hash_value = 0;
    for ( ; *s; s++)
        hash_value = 31 * hash_value + *s;
    
    DASSERT(hash_value > 0, "Invalid hash value: %s", str);
    
    return hash_value;
}

static RoutineInfo * merge_tuple(HChar * line_input, RoutineInfo * curr,
                                    HChar * report, aprof_report * r, 
                                    UInt * rid, Bool * invalid) {
    
    *invalid = False;
    
    if (VG_(strlen)(line_input) <= 0) return curr;
    HChar * line = put_delim(line_input);
    
    ASSERT(VG_(strlen)(line_input) + VG_(strlen)(report) + 1 < 2048,
        "line + report too long");
    HChar line_orig[2048] = {0};
    STR(line_orig, "\n\tReport: %s\n\tLine: %s\n", report, line_input);
    
    HChar * token = VG_(strtok)(line, DELIM_DQ);
    ASSERT(token != NULL, "Invalid line: %s", line_orig); 
    
    if (token[0] == 'r') {
        
        if (curr != NULL) {
            ASSERT(r->tmp > 0, "Invalid cumul: %s", line_orig);
        } else {
            ADD(r->performance_metric, r->tmp);
        }
        
        r->tmp = 0;
        
        // function name
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid fn name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        DASSERT(VG_(strlen)(token) > 0, "Invalid fn name: %s", line_orig); 
        HChar * name = VG_(strdup2)("fn_name", token);
        DASSERT(name != NULL, "Invalid fn name");
        
        // object name
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid obj name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        DASSERT(VG_(strlen)(token) > 0, "Invalid obj name: %s", line_orig); 
        HChar * obj_name = VG_(strdup2)("obj_name", token);
        DASSERT(obj_name != NULL, "Invalid obj name");
        
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        DASSERT(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(id, line_orig);
        
        *rid = id;
        
        // Search function
        UInt hash = str_hash(name);
        Function * fn = HT_lookup(r->fn_ht, hash);     
        while (fn != NULL && VG_(strcmp)(fn->name, name) != 0) {
            
            fn = fn->next;
        }
        
        if (fn == NULL) { // this is a new function
            
            fn = VG_(calloc)("fn", sizeof(Function), 1);
            DASSERT(fn != NULL, "Function not allocable");
            
            fn->key = hash;
            fn->name = name;
            
            HT_add_node(r->fn_ht, fn->key, fn);
            
        } else VG_(free)(name);
        
        if (fn->obj == NULL) { // new object
            
            UInt hash_obj = str_hash(obj_name);
            Object * obj = HT_lookup(r->obj_ht, hash_obj);
            while (obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0) {
                
                obj = obj->next;
            }
            
            if (obj == NULL) {
 
                obj = VG_(calloc)("obj", sizeof(Object), 1);
                DASSERT(obj != NULL, "Obj not allocable");
                
                obj->key = hash_obj;
                obj->name = obj_name;
                obj->filename = NULL; /* FixMe */
                HT_add_node(r->obj_ht, obj->key, obj);
                
                fn->obj = obj;
            
            } else VG_(free)(obj_name);
            
            fn->obj = obj;
        
        } else VG_(free)(obj_name);
        
        curr = HT_lookup(r->routine_hash_table, (UWord) fn);
        if (curr == NULL) {
            
            curr = new_routine_info(fn, (UWord) fn, r);
            DASSERT(curr != NULL, "Invalid routine info");
        
        } 
        
        UInt i;
        for (i = 0; i < sizeof(rtn_skip)/sizeof(HChar *); i++) {
            if (VG_(strcmp)(rtn_skip[i], curr->fn->name) == 0) {
                r->tmp = 1;
                break;
            }
        }

    } else if (token[0] == 'd') { 
        
        ASSERT(curr != NULL, "mangled name without a valid fn: %s", line_orig);
        
        //  routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        DASSERT(id > 0, "Invalid ID: %s", line_orig);
        UOF_LONG(id, line_orig);
        
        ASSERT(*rid == id, "Routine id mismatch: %s", line_orig);
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid mangled name: %s", line_orig);
        
        // function mangled name
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        
        DASSERT(VG_(strlen)(token) > 0, "Invalid mangled name: %s", line_orig); 
        
        if (curr->fn->mangled != NULL) {
            ASSERT(VG_(strcmp)(curr->fn->mangled, token) == 0, 
                        "different mangled: %s <=> %s", 
                        curr->fn->mangled, token);
        } else {
            
            curr->fn->mangled = VG_(strdup2)("mangled", (HChar *)token);
            DASSERT(curr->fn->mangled != NULL, "Invalid mangled name");
        
        }
        
    } else if (token[0] == 'p') {
        
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        ASSERT(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(id, line_orig);
        
        ASSERT(*rid == id, "Routine id mismatch: %s", line_orig);
        
        UInt i;
        for (i = 0; i < sizeof(rtn_skip)/sizeof(HChar *); i++) {
            if (VG_(strcmp)(rtn_skip[i], curr->fn->name) == 0) {
                VG_(free)(line);
                r->tmp = 1;
                return curr;
            }
        }

        // RMS
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid rms: %s", line_orig);
        ULong rms = VG_(strtoull10) (token, NULL);
        UOF_LONG(rms, line_orig);
        
        // min
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid min: %s", line_orig);
        ULong min = VG_(strtoull10) (token, NULL);
        ASSERT(min != LLONG_MIN && min != LLONG_MAX, 
                    "under/overflow: %s", line_orig);
        ASSERT(min > 0, "Invalid min: %s", line_orig);
        
        // max
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid max: %s", line_orig);
        ULong max = VG_(strtoull10) (token, NULL);
        UOF_LONG(max, line_orig);
        ASSERT(max > 0 && max >= min, "Invalid max: %s", line_orig);
        
        // sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid sum: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(sum, line_orig);
        ASSERT(sum >= max, "Invalid sum: %s", line_orig);
        
        // sqr sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid sqr sum: %s", line_orig);
        double sqr_sum = VG_(strtod) (token, NULL);
        UOF_DOUBLE(sqr_sum, line_orig);
        ASSERT(sqr_sum >= sum, "Invalid sqr_sum: %s", line_orig);
        
        // occ
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid occ: %s", line_orig);
        ULong occ = VG_(strtoull10) (token, NULL);
        UOF_LONG(occ, line_orig);
        ASSERT(occ > 0, "Invalid occ: %s", line_orig);
        
        ASSERT(sum >= min*occ, "Invalid sum: %s", line_orig);
        
        if (sqr_sum < ((double)min)*((double)min)*((double)occ)
                && !r->sqr_over) {
            
            EMSG(YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", 
                report);
            
            r->sqr_over = True;
        }
        
        // cumul_real
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid cumul: %s", line_orig);
        ULong cumul_real = VG_(strtoull10) (token, NULL);
        UOF_LONG(cumul_real, line_orig);
        ASSERT(cumul_real >= 0 && cumul_real <= sum, 
                    "Invalid cumul: %s", line_orig);
        
        ADD(r->tmp, cumul_real);
        
        // self_total
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self total: %s", line_orig);
        ULong self = VG_(strtoull10) (token, NULL);
        UOF_LONG(self, line_orig);
        ASSERT(self > 0 && self <= sum, "Invalid self total: %s", line_orig);
        
        // self_min
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self min: %s", line_orig);
        ULong self_min = VG_(strtoull10) (token, NULL);
        UOF_LONG(self_min, line_orig);
        ASSERT(self_min > 0 && self_min <= min, 
                    "Invalid self min: %s", line_orig);
        
        ASSERT(self >= self_min*occ, "Invalid self total: %s", line_orig);
        
        // self_max
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self max: %s", line_orig);
        ULong self_max = VG_(strtoull10) (token, NULL);
        UOF_LONG(self_max, line_orig);
        ASSERT(self_max >= self_min && self_max <= max,
                    "Invalid self max: %s", line_orig);
        
        // sqr self
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid sqr self: %s", line_orig);
        double self_sqr = VG_(strtod) (token, NULL);
        UOF_DOUBLE(self_sqr, line_orig);
        
        if (self_sqr < ((double)self_min) * ((double)self_min)
                && !r->self_sqr_over) {
            
            EMSG(YELLOW("Warning:") " Invalid self sqr (overflow?): %s\n", 
                report);
            
            r->self_sqr_over = True;
        }
        
        ULong rms_sum = 0;
        double rms_sqr = 0;
        ULong rvms_syscall_sum = 0;
        ULong rvms_thread_sum = 0;
        if (r->version == REPORT_VERSION && r->input_metric == RVMS) {
        
            // rms sum
            token = VG_(strtok)(NULL, DELIM_DQ);
            ASSERT(token != NULL, "Invalid rms sum: %s", line_orig);
            rms_sum = VG_(strtoull10) (token, NULL);
            UOF_LONG(rms_sum, line_orig);
            ASSERT(rms_sum <= rms*occ, "invalid rms sum: %s", line_orig);
            
            // ratio sum sqr
            token = VG_(strtok)(NULL, DELIM_DQ);
            ASSERT(token != NULL, "Invalid sqr rms: %s", line_orig);
            rms_sqr = VG_(strtod) (token, NULL);
            UOF_DOUBLE(rms_sqr, line_orig);
            ASSERT(rms_sqr <= ((double)rms) * ((double)rms) 
                        * ((double)occ), "invalid rms sqr: %s", line_orig);

            // rvms syscall sum
            token = VG_(strtok)(NULL, DELIM_DQ);
            if (token != NULL) {
                
                rvms_syscall_sum = VG_(strtoull10) (token, NULL);
                UOF_LONG(rvms_syscall_sum, line_orig);
                ASSERT(rvms_syscall_sum <= rms*occ, "invalid rvms syscall: %s", line_orig);
            
                // rvms thread sum
                token = VG_(strtok)(NULL, DELIM_DQ);
                ASSERT(token != NULL, "Invalid rvms thread: %s", line_orig);
                rvms_thread_sum = VG_(strtoull10) (token, NULL);
                UOF_LONG(rvms_thread_sum, line_orig);
                ASSERT(rvms_thread_sum <= rms*occ, "invalid rvms thread: %s", line_orig);
            
                ASSERT(rvms_thread_sum + rvms_syscall_sum <= rms * occ,
                            "invalid rvms syscall/thread: %s", line_orig);
            
            }          

        }

        ASSERT(curr != NULL, "Invalid routine: %s", line_orig);
        RMSInfo * info_access = HT_lookup(curr->rvms_map, rms); 
        if (info_access == NULL) {
            
            info_access = (RMSInfo * ) VG_(calloc)("rms_info", 1, sizeof(RMSInfo));
            DASSERT(info_access != NULL, "rms_info not allocable in function exit");
            
            info_access->min_cumulative_time = min;
            info_access->self_time_min = self_min;
            info_access->key = rms;
            HT_add_node(curr->rvms_map, info_access->key, info_access);
            
        }
        
        ADD(info_access->cumulative_time_sum, sum);
        ADD_D(info_access->cumulative_sum_sqr, sqr_sum);
        ADD(info_access->calls_number, occ);
        ADD(curr->total_calls, info_access->calls_number);

        ASSERT(info_access->cumulative_sum_sqr >= info_access->cumulative_time_sum, 
            "Invalid sqr_sum: %s", line_orig);

        if (info_access->max_cumulative_time < max) 
            info_access->max_cumulative_time = max;
    
        if (info_access->min_cumulative_time > min) 
            info_access->min_cumulative_time = min;
        
        ASSERT(info_access->max_cumulative_time >= 
                info_access->min_cumulative_time, "Invalid min/max");
        ASSERT(info_access->cumulative_time_sum >= 
                info_access->max_cumulative_time, "Invalid sum");
        
        ASSERT(info_access->cumulative_time_sum >= 
                info_access->min_cumulative_time *
                info_access->calls_number, 
                "Invalid sum");
                
        if (info_access->cumulative_sum_sqr < 
                ((double)info_access->min_cumulative_time) * 
                ((double)info_access->min_cumulative_time) *
                ((double)info_access->calls_number) &&
                !r->sqr_over) {
             
            EMSG(YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", 
                            report);
            
            r->sqr_over = True;
        }
        
        ADD(info_access->cumul_real_time_sum, cumul_real);
        ADD(r->real_total_cost, cumul_real);
        
        ADD(info_access->self_time_sum, self);
        ADD(r->self_total_cost, self);
        ADD_D(info_access->self_sum_sqr, self_sqr);
        
        ASSERT(info_access->cumul_real_time_sum >= 0 
                    && info_access->cumul_real_time_sum <= 
                    info_access->cumulative_time_sum, 
                    "Invalid cumul: %s", line_orig);
        
        if (info_access->self_time_min > self_min) 
            info_access->self_time_min = self_min;
    
        if (info_access->self_time_max < self_max) 
            info_access->self_time_max = self_max;
        
        ASSERT(info_access->self_time_min <= info_access->min_cumulative_time 
            && info_access->self_time_max <= info_access->max_cumulative_time
            && info_access->self_time_sum <= info_access->cumulative_time_sum,
                "Invalid self: %s", line_orig);
        
        ASSERT(info_access->self_time_max >= 
                info_access->self_time_min, "Invalid self min/max");
        ASSERT(info_access->self_time_sum >= 
                info_access->self_time_max, "Invalid self sum");
        
        ASSERT(info_access->self_time_sum >= 
                info_access->self_time_min *
                info_access->calls_number, 
                "Invalid sum");
                
        if (info_access->self_sum_sqr <
                ((double)info_access->self_time_min) * 
                ((double)info_access->self_time_min) *
                ((double)info_access->calls_number) &&
                !r->self_sqr_over) {

            EMSG(YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", 
                            report);
        }
        
        if (r->version == REPORT_VERSION && r->input_metric == RVMS) {
            
            ADD(info_access->rms_input_sum, rms_sum);
            ADD_D(info_access->rms_input_sum_sqr, rms_sqr);
            
            ASSERT(info_access->rms_input_sum <= 
                        rms * info_access->calls_number, 
                        "invalid rms sum");
            
            ASSERT(info_access->rms_input_sum_sqr <= 
                        ((double)rms) * ((double)rms) 
                        * ((double)info_access->calls_number), 
                        "invalid rms sqr");
        
            ADD(info_access->rvms_thread_sum, rvms_thread_sum);
            ADD(info_access->rvms_syscall_sum, rvms_syscall_sum);
            
            ASSERT( info_access->rvms_thread_sum + 
                    info_access->rvms_syscall_sum <= 
                    rms * info_access->calls_number,
                    "invalid rvms syscall/thread: %s", line_orig);
        
        }
    
    } else if (token[0] == 'g') {
    
        ASSERT(r->input_metric == RVMS, "Invalid report: rms but g - %s", line_orig);
    
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        DASSERT(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(id, line_orig);
        
        ASSERT(*rid == id, "Routine id mismatch: %s", line_orig);
        
        UInt i;
        for (i = 0; i < sizeof(rtn_skip)/sizeof(HChar *); i++) {
            if (VG_(strcmp)(rtn_skip[i], curr->fn->name) == 0) {
                VG_(free)(line);
                return curr;
            }
        }
        
        // RMS
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid rms: %s", line_orig);
        ULong rms = VG_(strtoull10) (token, NULL);
        UOF_LONG(rms, line_orig);
        
        // calls (optional)
        token = VG_(strtok)(NULL, DELIM_DQ);
        ULong calls = 0;
        if (token != NULL)
            calls = VG_(strtoull10) (token, NULL);
        UOF_LONG(calls, line_orig);
            
        ASSERT(curr != NULL, "distinct rms for a void routine: %s", line_orig);
        RMSValue * node = (RMSValue *) HT_lookup(curr->distinct_rms, rms);
        if (node == NULL) {
            
            node = VG_(calloc)("distinct rms node", sizeof(RMSValue), 1);
            node->key = rms;
            node->calls = calls;
            HT_add_node(curr->distinct_rms, node->key, node);
        
        } else node->calls += calls;

    } else if (token[0] == 'a') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL && VG_(strlen)(token) > 0, 
            "invalid cmd name: %s", line_orig); 
        HChar app[1024] = { 0 };
        while (token != NULL) {
            
            ASSERT(VG_(strlen)(app) + VG_(strlen)(token) + 1 < 1024,
                "cmd name too long: %s", line_orig); 
            
            VG_(strcat)(app, token);
            VG_(strcat)(app, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
        
        }
         
        // remove final space
        app[VG_(strlen)(app) -1] = '\0';
        
        if (r->cmd == NULL) {
            
            HChar * app_c = VG_(strdup2)("app", app);
            r->app = app_c;
        
        } else {
            
            if (!merge_all && VG_(strcmp)(app, r->app) != 0) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
            
            /*
            ASSERT(VG_(strcmp)(app, r->app) == 0, 
                "different app");
            */
        }
    
    } else if (token[0] == 'k') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid perf metric: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(sum, line_orig);
        ASSERT(sum > 0, "Invalid sum: %s", line_orig);
        
        /*
         * merge_tuple is used also to test if a report needs
         * to be merged with the current loaded report. We need to
         * check report tag f so we sum (maybe) the performance metric
         * when we see the first routine
         */
        r->tmp = 0;
        ADD(r->tmp, sum);
        
    } else if (token[0] == 'v') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid version: %s", line_orig);
        ULong ver = VG_(strtoull10) (token, NULL);
        UOF_LONG(ver, line_orig);
        ASSERT(ver == REPORT_VERSION || ver == REPORT_VERSION_OLD, 
            "Invalid version: %s", line_orig);
        
        if (r->version == 0) r->version = ver;
        if (merge_runs || merge_threads || compare){
            
            if (compare && r == &ap_rep[1]) {
                
                /*
                ASSERT(ap_rep[0].version == r->version, 
                "You are elaborating reports of different versions: %s", 
                line_orig);
                */
                
            } else {
                
                if (r->version != ver) {
                    if (merge_all) EMSG("reports with different versions");
                    *invalid = True;
                    VG_(free)(line);
                    return NULL;
                }
                
                /*
                ASSERT(r->version == ver, 
                "You are elaborating reports of different versions: %s", 
                line_orig);
                */
            }
            
        }
        
        if (r->version == REPORT_VERSION_OLD)
            r->input_metric = RMS;
        
    } else if (token[0] == 'q' || token[0] == 'x') {
        
        ASSERT(0, "Merge of reports with CCT is not yet supported");
        
    } else if (token[0] == 'i'){ 
    
        if (r->version == REPORT_VERSION) {
            
            token = VG_(strtok)(NULL, DELIM_DQ);
            ASSERT(token != NULL, "Invalid input metric: %s", line_orig);
            
            if (VG_(strcmp)("rms", token) == 0) {
                
                if (r->input_metric > 0)
                    ASSERT(r->input_metric == RMS, "Invalid metric")
                    
                r->input_metric = RMS;
                
            } else if (VG_(strcmp)("rvms", token) == 0) {
                
                if (r->input_metric > 0)
                    ASSERT(r->input_metric == RVMS, "Invalid metric")
                
                r->input_metric = RVMS;
            
            } else
                ASSERT(0, "Invalid input metric: %s", line_orig);
        
        } else if (r->version == REPORT_VERSION_OLD)
            r->input_metric = RMS;
        else
            ASSERT(0, "Invalid metric: %s", line_orig);
    
    } else if (token[0] == 'c' || token[0] == 'e' || token[0] == 'm'
                    || token[0] == 'u'){ 
    
        // ignored...
    
    } else if(token[0] == 't') {
    
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid memory resolution: %s", line_orig);
        UInt m_res = VG_(strtoull10) (token, NULL);
        ASSERT(m_res == 1 || m_res == 2 || m_res == 4 || m_res == 8 || m_res == 16,
            "Invalid memory resolution: %s", line_orig);
    
        if (r->memory_resolution != m_res) {
            if (merge_all) 
                EMSG("reports with different memory resolutions");
            *invalid = True;
            VG_(free)(line);
            return NULL;
        }
        
    } else if (token[0] == 'f') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL && VG_(strlen)(token) > 0, 
            "invalid cmd name: %s", line_orig); 
        HChar app[1024] = { 0 };
        while (token != NULL) {
            
            ASSERT(VG_(strlen)(app) + VG_(strlen)(token) + 1 < 1024,
                "cmd name too long: %s", line_orig); 
            
            VG_(strcat)(app, token);
            VG_(strcat)(app, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
        
        }
         
        // remove final space
        app[VG_(strlen)(app) -1] = '\0';
        
        if (r->cmd == NULL) {
            
            HChar * app_c = VG_(strdup2)("cmd", app);
            r->cmd = app_c;
        
        } else {
            
            if (!merge_all && VG_(strcmp)(app, r->cmd) != 0) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
            
            /*
            ASSERT(VG_(strcmp)(app, r->cmd) == 0, 
                "different command");
            */
        }
        
    } else {
        
        ASSERT(0, "Unknown tag %c %s", token[0], line_orig);
        
    }
    
    VG_(free)(line);
    return curr;
}

static void check_rvms(double sum_rms, double sum_rvms, 
                            ULong num_rms, ULong num_rvms,
                            HChar * name, HChar * report) {
    
    double ext_ratio = 0;
            
    ASSERT(sum_rms >= 0, "Invalid sum RMS - %s - %s", 
                name, report);
    ASSERT(sum_rvms >= sum_rms, 
            "RVMS should be bigger than RMS - %s - %s",
            name, report);
    
    if (sum_rvms == 0 && sum_rms == 0)
        ext_ratio = 0;
    else 
        ext_ratio = 1.0 - (sum_rms/sum_rvms);
    
    // # RVMS - # RMS
    ULong diff = num_rvms - num_rms; 

    if (ext_ratio == 0) {
        ASSERT(diff == 0, 
            "Routine %s has external ratio %f and diff # %llu - %s",
            name, ext_ratio, diff, report);
    }
                                              
}

static void post_merge_consistency(aprof_report * r, HChar * report) {
    
    HT_ResetIter(r->routine_hash_table);
    RoutineInfo * rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
    while (rtn != NULL) {
        
        ULong cumul_real = 0;
        ULong sum_rms = 0;
        ULong sum_rvms = 0;
        
        UInt k;
        for (k = 0; k < sizeof(rtn_skip)/sizeof(HChar *); k++) {
            if (VG_(strcmp)(rtn_skip[k], rtn->fn->name) == 0) {
                rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
                break;
            }
        }
        
        if (rtn == NULL) break;
        HT_ResetIter(rtn->rvms_map);
        RMSInfo * i = (RMSInfo *) HT_Next(rtn->rvms_map);
        while (i != NULL) {
            
            if (r->version == REPORT_VERSION) {
                ADD(sum_rms, i->rms_input_sum);
                ADD(sum_rvms, i->key*i->calls_number);
            }
            
            ADD(cumul_real, i->cumul_real_time_sum);
            i = (RMSInfo *) HT_Next(rtn->rvms_map);
        }
        
        ASSERT(cumul_real > 0, "Invalid cumul real: %s:%s", 
                    rtn->fn->name, report);
        
        if (r->version == REPORT_VERSION && r->input_metric == RVMS) {
            
            check_rvms(sum_rms, sum_rvms, 
                        HT_count_nodes(rtn->distinct_rms),
                        HT_count_nodes(rtn->rvms_map),
                        rtn->fn->name, report);
        
            ULong sum_rms_distinct = 0;
            HT_ResetIter(rtn->distinct_rms);
            RMSValue * v = NULL;
            while (1) {
                v = (RMSValue *) HT_Next(rtn->distinct_rms);
                if (v == NULL) break;
                
                ADD(sum_rms_distinct, v->key * v->calls);
            }
            ASSERT(sum_rms_distinct == sum_rms, 
                "Mismatch sum rms (tag v versus rvms in %s %s",
                    rtn->fn->name, report)
            
        }
        rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
    }
    
}

static Bool merge_report(HChar * report, aprof_report * rep_data) {
    
    //VG_(printf)("Try to merge: %s\n", report);
    HChar buf[4096];
    
    UInt m_res = get_memory_resolution_report(report);
    ASSERT(m_res == 1 || m_res == 2 || m_res == 4 || m_res == 8 || m_res == 16,
            "Invalid memory resolution: %s", report);
    
    if (rep_data->memory_resolution > 0 && rep_data->memory_resolution != m_res) 
        return False;

    rep_data->memory_resolution = m_res;
    
    Int file = VG_(open)(report, O_RDONLY, S_IRUSR|S_IWUSR);
    if (file < 0) perror(NULL);
    ASSERT(file >= 0, "Can't read: %s", report);
    
    HChar line[1024];
    UInt offset = 0;
    RoutineInfo * current_routine = NULL;
    UInt curr_rid = 0;
    Bool invalid = False;
    
    rep_data->sqr_over = False;
    rep_data->self_sqr_over = False;
    
    /* merge tuples */
    while (1) {
        
        Int r = VG_(read)(file, buf, 4096);
        if (r < 0) {
            
            EMSG("Error when reading %s\n", report);
            return False;
        
        } else if (r == 0) break; /* EOF */
        
        Int i;
        for (i = 0; i < r; i++) {
            
            if (buf[i] == '\n') {
                
                line[offset++] = '\0';
                current_routine = merge_tuple(line, current_routine, 
                                                report, rep_data, 
                                                &curr_rid, &invalid);
                offset = 0;
                
                if (invalid) {
                    VG_(close)(file);
                    return False;
                }
            
            } else line[offset++] = buf[i]; 
            
            ASSERT(offset < 1024, "Line too long");
        }
        
    }    
    
    line[offset++] = '\0';
    current_routine = merge_tuple(line, current_routine, report, 
                                        rep_data, &curr_rid, &invalid);

    VG_(close)(file);
    
    if (invalid) return False;
    
    post_merge_consistency(rep_data, report);
    
    return True;
} 


static HChar ** search_reports(UInt * n) {
    
    ASSERT(directory != NULL, "Invalid directory");
    UInt size = 256;
    HChar ** reports = VG_(calloc)("reports", sizeof(HChar *), size);
    DASSERT(reports != NULL, "reports not allocable");
    
    DIR * dir = opendir(directory);
    ASSERT(dir != NULL, "Can't open directory: %s\n", directory);

    *n = 0;
    while (1) {
        
        struct dirent * file = readdir(dir);
        if (file == NULL) break;
        
        if (file->d_type == DT_REG &&
            strcmp(".aprof", file->d_name + strlen(file->d_name) - 6) == 0) {
            
            //printf("pid: %d\n", get_pid_report(file->d_name));
            //printf("resolution: %d\n", get_memory_resolution_report(file->d_name));
            
            if (*n + 1 == size) {
                size = size * 2;
                reports = realloc(reports, sizeof(HChar *) * size);
            }
            HChar * rep = malloc(1024);
            //printf("directory: %s\n", directory);
            STR(rep, "%s%s", directory, file->d_name);
            reports[(*n)++] = rep;
            
        }
        
    }

    closedir(dir);

    return reports;

}

static void print_diff(double a, double b) {
    
    double diff = b - a;
    double diff_p = 0;
    if (a != 0) diff_p = diff / (a / 100);
    
    printf(" | %*.0f | %*.0f | ", NUM_ALIGN*3, a, NUM_ALIGN*3, b);
    
    if ((a == 0 && b == 0) || diff == 0) printf("[%+*.1f%%]", NUM_ALIGN, 0.0);
    else if (a == 0) printf("[" GREEN("%*s%%") "]", NUM_ALIGN, "+inf");
    else if (b == 0) printf("[" RED("%*s%%") "]", NUM_ALIGN, "-inf");
    else if (diff > 0) printf("[" GREEN("%+*.1f%%") "]", NUM_ALIGN, diff_p);
    else printf("[" RED("%+*.1f%%") "]", NUM_ALIGN, diff_p);
    
    printf("\n");
    
}

static void missing_routines(aprof_report * r, aprof_report * r2, 
                                        UInt mode, Bool reverse) {

    HChar b[1024];

    HT_ResetIter(r->routine_hash_table);
    RoutineInfo * rtn = NULL;
    while (1) {
        
        rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
        if (rtn == NULL) break;
        
        Function * fn = (Function *) HT_lookup(r2->fn_ht, (UWord) rtn->fn->key);
        
        if (fn == NULL) {
            
            STR(b, "Missing routine [%s]", rtn->fn->name);
            printf("%-*s", STR_ALIGN, b);
            if (reverse)
                print_diff(0, 1);
            else
                print_diff(1, 0);
        
            continue;
        }
        
        RoutineInfo * rtn2 = (RoutineInfo *) HT_lookup(r2->routine_hash_table, 
                                                        (UWord) fn);
        
        if (rtn2 == NULL) {
            
            STR(b, "Missing routine [%s]", rtn->fn->name);
            printf("%-*s", STR_ALIGN, b);
            if (reverse)
                print_diff(0, 1);
            else
                print_diff(1, 0);
        
        }
    }

}

static Bool check_tollerance(double a, double b, ULong toll, ULong toll_p) {
    
    double diff = a - b; 
    
    if (diff > 0 && diff > toll && diff > (b / 100) * toll_p) 
        return True;
    
    else if (diff < 0 && -diff > toll && -diff > (a / 100) * toll_p)
        return True;
        
    return False;
}

static Bool check_input_tol(double a, double b) {
    return check_tollerance(a, b, INPUT_TOLLERANCE, INPUT_TOLLERANCE_P);
}

static Bool check_cost_tol(double a, double b) {
    return check_tollerance(a, b, COST_TOLLERANCE, COST_TOLLERANCE_P);
}

static void print_diff_if_input(double a, double b, HChar * str) {
    
    if (check_input_tol(a, b)) {
        
        printf("%-*s", STR_ALIGN, str);
        print_diff(a, b);
        
    }

}

static void print_diff_if_cost(double a, double b, HChar * str) {
    
    if (check_cost_tol(a, b)) {
        
        printf("%-*s", STR_ALIGN, str);
        print_diff(a, b);
        
    }
    
}

static int compar(const void * a, const void * b) {
    
    ULong c = *((ULong *) a);
    ULong d = *((ULong *) b);
    
    if (c == d) return 0;
    if (c < d) return -1;
    else return 1;
}

static void compare_distinct_rms(RoutineInfo * rtn, RoutineInfo * rtn2,
                                    aprof_report * r, aprof_report * r2,
                                    UInt mode) {
    
    HChar b[1024];
    
    HashTable * ht1 = rtn->rvms_map;
    HashTable * ht2 = rtn2->rvms_map;
    
    if (r->input_metric == RVMS)
        ht1 = rtn->distinct_rms;
        
    if (r2->input_metric == RVMS)
        ht2 = rtn2->distinct_rms;
    
    ULong * rms1 = malloc(sizeof(ULong) * HT_count_nodes(ht1));
    ULong * rms2 = malloc(sizeof(ULong) * HT_count_nodes(ht2));
    
    UInt k = 0;
    
    HT_ResetIter(ht1);
    HashNode * n1 = NULL;
    while(1) {
            
        n1 = HT_Next(ht1);
        if (n1 == NULL) break;
        rms1[k++] = n1->key;
            
    }
    
    HT_ResetIter(ht2);
    n1 = NULL;
    k = 0;
    while(1) {
            
        n1 = HT_Next(ht2);
        if (n1 == NULL) break;
        rms2[k++] = n1->key;
            
    }
    
    qsort(rms1, HT_count_nodes(ht1), sizeof(ULong), compar);
    qsort(rms2, HT_count_nodes(ht2), sizeof(ULong), compar);
    
    for (k = 0; k < HT_count_nodes(ht1); k++) {
        
        n1 = HT_lookup(ht1, (UWord) rms1[k]);
        ASSERT(n1 != NULL, "Impossible");
        
        HashNode * n2 = NULL;
        if (k < HT_count_nodes(ht2))
            n2 = HT_lookup(ht2, (UWord) rms1[k]);
        
        if (n2 == NULL) {
            
            IFS(1) {
                
                STR(b, "Missing RMS [%s]", rtn->fn->name);
                printf("%-*s", STR_ALIGN, b);
                print_diff(n1->key, 0);
            
            }
            
        }
        
    }
    
    for (k = 0; k < HT_count_nodes(ht2); k++) {
        
        n1 = HT_lookup(ht2, (UWord) rms2[k]);
        ASSERT(n1 != NULL, "Impossible");
        
        HashNode * n2 = NULL;
        if (k < HT_count_nodes(ht1))
            n2 = HT_lookup(ht1, (UWord) rms2[k]);
        
        if (n2 == NULL) {
            
            IFS(1) {
                
                STR(b, "Missing RMS [%s]", rtn->fn->name);
                printf("%-*s", STR_ALIGN, b);
                print_diff(0, n1->key);
            
            }
            
        }
        
    }
    
    VG_(free)(rms1);
    VG_(free)(rms2);
}

static void compare_routine(RoutineInfo * rtn, RoutineInfo * rtn2, 
                                aprof_report * r, aprof_report * r2,
                                UInt mode) {

    HChar b[1024];
    
    ULong sum_rvms = 0;
    ULong sum_rvms2 = 0;
    //ULong max_diff_rvms = 0;
    
    ULong sum_real_cost = 0;
    ULong sum_real_cost2 = 0;
    //ULong max_diff_rcost = 0;
    
    ULong sum_self_cost = 0;
    ULong sum_self_cost2 = 0;
    //ULong max_diff_scost = 0;
    
    ULong calls = 0;
    ULong calls2 = 0;
    //ULong max_diff_calls = 0;
    
    ULong sum_rms = 0;
    ULong sum_rms2 = 0;
    //ULong max_diff_rms = 0;

    if (rtn->total_calls != rtn2->total_calls) {
    
        STR(b, "# calls [%s]", rtn->fn->name);
        printf("%-*s", STR_ALIGN, b);
        print_diff(rtn->total_calls, rtn2->total_calls);
        
    }

    /* we check for missing RVMS later one by one...
    if (HT_count_nodes(rtn->rvms_map) != HT_count_nodes(rtn2->rvms_map)) {
    
        STR(b, "# RVMS [%s]", rtn->fn->name);
        printf("%-*s", STR_ALIGN, b);
        print_diff(HT_count_nodes(rtn->rvms_map), HT_count_nodes(rtn2->rvms_map));
        
    }
    */
    
    /* we check distinct rms in more detail later...
    if (r->input_metric == RVMS && r2->input_metric == RVMS &&
        HT_count_nodes(rtn->distinct_rms) != HT_count_nodes(rtn2->distinct_rms)) {
    
        STR(b, "# RMS [%s]", rtn->fn->name);
        printf("%-*s", STR_ALIGN, b);
        print_diff(HT_count_nodes(rtn->distinct_rms), HT_count_nodes(rtn2->distinct_rms));
        
    }
    */
    
    UInt k = 0; UInt j = 0;
    
    ULong * rms_1 = malloc(HT_count_nodes(rtn->rvms_map) * sizeof(ULong));
    ULong * rms_2 = malloc(HT_count_nodes(rtn2->rvms_map) * sizeof(ULong));

    // Collect RMS of rtn
    HT_ResetIter(rtn->rvms_map);
    RMSInfo * i = NULL;
    while (1) {
        
        i = (RMSInfo *) HT_Next(rtn->rvms_map);
        if (i == NULL) break;
        
        rms_1[k++] = i->key;
    }
    
    // Collect RMS of rtn2
    HT_ResetIter(rtn2->rvms_map);
    i = NULL; k = 0;
    while (1) {
        
        i = (RMSInfo *) HT_Next(rtn2->rvms_map);
        if (i == NULL) break;
        
        rms_2[k++] = i->key;
    }
    
    // sort
    qsort(rms_1, HT_count_nodes(rtn->rvms_map), sizeof(ULong), compar);
    qsort(rms_2, HT_count_nodes(rtn2->rvms_map), sizeof(ULong), compar);

    ULong call = 0;
    ULong call2 = 0;
    for (k = 0, j = 0; k < HT_count_nodes(rtn->rvms_map); k++) {
        
        i = (RMSInfo *) HT_lookup(rtn->rvms_map, rms_1[k]);
        if (i == NULL) ASSERT(0, "Impossible");
        
        ADD(sum_rvms, i->key * i->calls_number);
        ADD(calls, i->calls_number);
        ADD(sum_real_cost, i->cumul_real_time_sum);
        ADD(sum_self_cost, i->self_time_sum);
        ADD(sum_rms, i->rms_input_sum);
        
        if (r->input_metric != r2->input_metric) continue;
        
        RMSInfo * i2 = NULL;
        if (j < HT_count_nodes(rtn2->rvms_map)) {
        
            i2 = (RMSInfo *) HT_lookup(rtn2->rvms_map, rms_2[j]);
            ASSERT(i2 != NULL, "Impossible");
        
        }
        
        if (i2 == NULL) {

            IFS(1) {
                
                STR(b, "Missing RVMS [%s]", rtn->fn->name);
                printf("%-*s", STR_ALIGN, b);
                print_diff(i->key, 0);
            
            }
        
        } else if (i->key != i2->key) {
            
            IFS(1) {
                
                STR(b, "Different RVMS [%s]", rtn->fn->name);
                print_diff_if_input(i->key, i2->key, b);
            
            }
            
        }
        
        ADD(call, i->calls_number);
        if (i2 != NULL && call >= call2 + i2->calls_number) {
            ADD(call2, i2->calls_number);
            j++;
        }
    
    }
    
    call = 0;
    call2 = 0;
    
    // Missing tuples in rtn
    for (k = 0, j = 0; k < HT_count_nodes(rtn2->rvms_map); k++) {
        
        i = (RMSInfo *) HT_lookup(rtn2->rvms_map, rms_2[k]);
        if (i == NULL) ASSERT(0, "Impossible");
        
        ADD(sum_rvms2, i->key * i->calls_number);
        ADD(calls2, i->calls_number);
        ADD(sum_real_cost2, i->cumul_real_time_sum);
        ADD(sum_self_cost2, i->self_time_sum);
        ADD(sum_rms2, i->rms_input_sum);
        
        if (r->input_metric != r2->input_metric) continue;
        
        RMSInfo * i2 = NULL;
        if (j < HT_count_nodes(rtn->rvms_map)) {
            
            i2 = (RMSInfo *) HT_lookup(rtn->rvms_map, rms_1[j]);
            ASSERT(i2 != NULL, "Impossible");
            
        }
        
        if (i2 == NULL) {

            IFS(1) {
                
                STR(b, "Missing RVMS [%s]", rtn->fn->name);
                printf("%-*s", STR_ALIGN, b);
                print_diff(0, i->key);
            
            }
        
        } else if (i->key != i2->key) {
            
            IFS(1) {
                
                STR(b, "Different RVMS [%s]", rtn->fn->name);
                print_diff_if_input(i2->key, i->key, b);
            
            }
        
        }
        
        ADD(call, i->calls_number);
        if (i2 != NULL && call >= call2 + i2->calls_number) {
            ADD(call2, i2->calls_number);
            j++;
        }
        
    }

    if (r->input_metric != r2->input_metric) {
    
        STR(b, "Diff sum(RVMS) [%s]", rtn->fn->name);
        print_diff_if_input(sum_rvms, sum_rvms2, b);    
    
    }
    
    if (True) {
        
        STR(b, "Diff sum(calls) [%s]", rtn->fn->name);
        print_diff_if_cost(calls, calls2, b); 

        STR(b, "Diff sum(real_cost) [%s]", rtn->fn->name);
        print_diff_if_cost(sum_real_cost, sum_real_cost2, b);

        STR(b, "Diff sum(self_cost) [%s]", rtn->fn->name);
        print_diff_if_cost(sum_self_cost, sum_self_cost2, b);
    
    }
    
    VG_(free)(rms_1);
    VG_(free)(rms_2);

}

static void compare_report(aprof_report * r, aprof_report * r2, UInt mode) {
    
    //printf("Comparing reports:\n\n");
    
    UInt i = 128;
    while (i-- > 0) printf("-");
    printf("\n");
    
    printf("%*s | %*s | %*s | %*s\n", STR_ALIGN, "Report", 
                NUM_ALIGN*3, basename(logs[0]),
                NUM_ALIGN*3, basename(logs[1]),
                NUM_ALIGN + 2, "diff(%)");
    
    i = 128;
    while (i-- > 0) printf("-");
    printf("\n");
    
    printf("%*s | %*s | %*s | \n", STR_ALIGN, "", 
                NUM_ALIGN*3, "",
                NUM_ALIGN*3, "");
    
    printf("%-*s", STR_ALIGN, "Version:");
    printf(" | %*u | %*u | \n", NUM_ALIGN*3, r->version, 
            NUM_ALIGN*3, r2->version);
    
    printf("%-*s", STR_ALIGN, "Input metric:");
    
    if (r->input_metric == RMS)
        printf(" | %*s |", NUM_ALIGN*3, "RMS");
    else
        printf(" | %*s |", NUM_ALIGN*3, "RVMS");
    
    if (r2->input_metric == RMS)
        printf(" %*s | \n", NUM_ALIGN*3, "RMS");
    else
        printf(" %*s | \n", NUM_ALIGN*3, "RVMS");
    
    // # routines
    printf("%-*s", STR_ALIGN, "# routines:");
    print_diff(HT_count_nodes(r->routine_hash_table),
                HT_count_nodes(r2->routine_hash_table));
    
    // total metric
    printf("%-*s", STR_ALIGN, "Performance metric:");
    print_diff(r->performance_metric, r2->performance_metric);
    
    /* not so useful...
    // real total cost
    printf("%-*s", STR_ALIGN, "Real cumulativetotal cost:");
    print_diff(r->real_total_cost, r2->real_total_cost);
    */
    
    // self total cost
    printf("%-*s", STR_ALIGN, "Self total cost:");
    print_diff(r->self_total_cost, r2->self_total_cost);
    
    missing_routines(r, r2, mode, False);
    missing_routines(r2, r, mode, True);

    HT_ResetIter(r->routine_hash_table);
    RoutineInfo * rtn = NULL;
    while (1) {
        
        rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
        if (rtn == NULL) break;
        
        Function * fn = (Function *) HT_lookup(r2->fn_ht, (UWord) rtn->fn->key);
        while (fn != NULL && VG_(strcmp)(fn->name, rtn->fn->name) != 0) {
            fn = fn->next;
        }
        if (fn == NULL) continue;
        
        RoutineInfo * rtn2 = (RoutineInfo *) HT_lookup(r2->routine_hash_table, 
                                                        (UWord) fn);
        if (rtn2 == NULL) continue;
        
        // compare routine
        compare_routine(rtn, rtn2, r, r2, mode);
        
        if (r->input_metric == RVMS || r2->input_metric == RVMS)
            compare_distinct_rms(rtn, rtn2, r, r2, mode);
            
    }
    
    return;
} 

static void save_report(aprof_report * r, HChar * report_name) {
    
    /*
    char cmd[1024] = {0};
    sprintf(cmd, "touch %s", report_name);
    //printf("%s\n", cmd);
    int res = system(cmd);
    ASSERT(res != -1, "Error during copy");
    return;
    */
    
    FILE * report = fopen(report_name, "w");
    ASSERT(report != NULL, "Can't save current report into %s", report_name);
    
    // header
    fprintf(report, "c -------------------------------------\n");
    fprintf(report, "c report generated by aprof (valgrind) \n");
    fprintf(report, "c -------------------------------------\n");
    
    // write version 
    fprintf(report, "v %d\n", r->version);
    
    // Maximum value for the metric
    fprintf(report, "k %llu\n", r->performance_metric);
    
    // write mtime binary
    fprintf(report, "e %d\n", 0);
    
    // write performance metric type
    fprintf(report, "m bb-count\n");
    
    // write input metric type
    if (r->input_metric == RMS)
        fprintf(report, "i rms\n");
    else
        fprintf(report, "i rvms\n");
    
    // write memory resolution
    fprintf(report, "t %d\n", r->memory_resolution);
    
    // write application name
    fprintf(report, "a %s\n", r->app);
    
    // write commandline
    fprintf(report, "f %s\n", r->cmd);
    
    HT_ResetIter(r->routine_hash_table);

    RoutineInfo * rtn_info = HT_Next(r->routine_hash_table);
    while (rtn_info != NULL) {
        
        char * obj_name = "NONE";
        if (rtn_info->fn->obj != NULL)
            obj_name = rtn_info->fn->obj->name; 
        
        // routine
        fprintf(report, "r \"%s\" \"%s\" %llu\n", 
                    rtn_info->fn->name, obj_name, 
                    rtn_info->routine_id);
        
        // mangled name
        if (rtn_info->fn->mangled != NULL) {
            fprintf(report, "u %llu \"%s\"\n", rtn_info->routine_id, 
                        rtn_info->fn->mangled);
        }

        // tuples 
        HT_ResetIter(rtn_info->rvms_map);
        RMSInfo * info_access = HT_Next(rtn_info->rvms_map);
        while (info_access != NULL) {
            
            if (r->input_metric == RVMS) {
                
                fprintf(report,
                            "p %llu %lu %llu %llu %llu %.0f %llu %llu %llu %llu %llu %.0f %llu %.0f %llu %llu\n", 
                            rtn_info->routine_id,
                            info_access->key,
                            info_access->min_cumulative_time,
                            info_access->max_cumulative_time,
                            info_access->cumulative_time_sum, 
                            info_access->cumulative_sum_sqr,
                            info_access->calls_number,
                            info_access->cumul_real_time_sum,
                            info_access->self_time_sum,
                            info_access->self_time_min,
                            info_access->self_time_max,
                            info_access->self_sum_sqr,
                            info_access->rms_input_sum,
                            info_access->rms_input_sum_sqr,
                            info_access->rvms_syscall_sum,
                            info_access->rvms_thread_sum
                            );
            
            } else {
                
                fprintf(report,
                            "p %llu %lu %llu %llu %llu %.0f %llu %llu %llu %llu %llu %.0f\n",
                            rtn_info->routine_id,
                            info_access->key,
                            info_access->min_cumulative_time,
                            info_access->max_cumulative_time,
                            info_access->cumulative_time_sum, 
                            info_access->cumulative_sum_sqr,
                            info_access->calls_number,
                            info_access->cumul_real_time_sum,
                            info_access->self_time_sum,
                            info_access->self_time_min,
                            info_access->self_time_max,
                            info_access->self_sum_sqr
                            );
                
            }
            
            
            info_access = HT_Next(rtn_info->rvms_map);
        }
        
        HT_ResetIter(rtn_info->distinct_rms);
        RMSValue * node = (RMSValue *) HT_Next(rtn_info->distinct_rms);
        while(node != NULL) {
            
            fprintf(report, "g %llu %lu %llu\n", 
                                rtn_info->routine_id, node->key,
                                node->calls);
            node = HT_Next(rtn_info->distinct_rms);
        
        }
    
        rtn_info = HT_Next(r->routine_hash_table);

    }
    
    fclose(report);
}

static void clean_data(aprof_report * rep) {
    
    ASSERT(rep != NULL, "Invalid aprof report");
    
    if (rep->fn_ht != NULL) HT_destruct(rep->fn_ht);
    if (rep->obj_ht != NULL) HT_destruct(rep->obj_ht);
    if (rep->routine_hash_table != NULL) HT_destruct(rep->routine_hash_table);
    if (rep->cmd != NULL) VG_(free)(rep->cmd);
    if (rep->app != NULL) VG_(free)(rep->app);
    
}

static void reset_data(aprof_report * rep) {
    
    clean_data(rep);
    
    rep->next_routine_id = 0;
    rep->cmd = NULL;
    rep->app = NULL;
    rep->memory_resolution = 0;
    rep->fn_ht = HT_construct(fn_destroy);
    rep->obj_ht = HT_construct(obj_destroy);
    rep->routine_hash_table = HT_construct(destroy_routine_info);
    rep->version = 0;
    rep->input_metric = 0;
    rep->performance_metric = 0;
    rep->real_total_cost = 0;
    rep->self_total_cost = 0;
    rep->sum_distinct_rms = 0;
    rep->tmp = 0;
    
    return;
    
}

static HChar ** merge_by_run(HChar ** reports, UInt * size, 
                                Bool merged_by_thread) {
    
    if (merge_all) printf("Merging all reports...\n");
    else printf("Merging reports with same command...\n");
    if (*size == 0) return reports;
    
    UInt size_post = 0; UInt i = 0;
    HChar ** reports_post = VG_(calloc)("test", sizeof(HChar *), *size);
    
    /*
     * Merge by command
     */
    char merge_dir[1024] = {0};
    int res = 0;
    HChar buf[1024] = {0};
    STR(buf, "%s", reports[0]); 
    if (merge_threads)
        res = sprintf(merge_dir, "%s/../%s", dirname(buf),
                        DIR_MERGE_BOTH);
    else if (merge_runs)
        res = sprintf(merge_dir, "%s/%s", dirname(buf),
                        DIR_MERGE_RUNS);
    else
        res = sprintf(merge_dir, "%s/%s", dirname(buf),
                        DIR_MERGE_ALL);
    
    ASSERT(res < 1024, "path too long");
    res = mkdir(merge_dir, 0777);
    ASSERT(errno != EEXIST, "directory %s already exists", merge_dir);
    ASSERT(res == 0, "Invalid merge directory");
    
    Int curr = -1; UInt merged = 0;
    UInt curr_pid = 0, curr_tid = 0;
    while (1) {
        
        if (reports[i] == NULL) goto next;
        
        //printf("checking report: %s\n", reports[i]);
        
        if (curr == -1) {
            
            curr = i;
            curr_pid = get_pid_report(reports[i]);
            curr_tid = get_tid_report(reports[i]);
            
            reset_data(&ap_rep[0]);
            //printf("Trying with %s\n", reports[curr]);
            Bool res = merge_report(reports[curr], &ap_rep[0]);
            ASSERT(res, "Report %s should be merged but is invalid", 
                                reports[curr]);
            merged++;
            
        } else {
            
            ASSERT(merged > 0, "Impossible");
             
            /*
            printf("Current: %s [%u:%u] - Checking: %s [%u:%u]\n", 
                        reports[curr], curr_pid, curr_pid, reports[i],
                        get_pid_report(reports[i]), get_tid_report(reports[i]));
            */
            if (merge_all || (curr_pid != get_pid_report(reports[i]) &&
                    (merge_threads || curr_tid == get_tid_report(reports[i]))) ) {

                /*
                printf("Current: %s [%u:%u] - Checking: %s [%u:%u]\n", 
                            reports[curr], curr_pid, curr_tid, reports[i],
                            get_pid_report(reports[i]), get_tid_report(reports[i]));
                */
                
                Bool res = merge_report(reports[i], &ap_rep[0]);
                if (res) {
                    if (merged == 1) 
                        printf("Merging: %s %s ", reports[curr], reports[i]);
                    else
                        printf("%s ", reports[i]);
                    free(reports[i]);
                    reports[i] = NULL;
                    merged++;
                }
            }
            
        }

next:            
        if (i + 1 == *size) {
            
            if (curr == -1) break;
            
            if (merged > 0) {
                
                HChar * new_rep = VG_(calloc)("t", 1024, 2);
                HChar buf[1024] = {0};
                STR(buf, "%s", reports[curr]); 
                //printf("saving %s\n", reports[curr]);
                if (merge_threads)
                    sprintf(new_rep, "%s/../%s/%s", dirname(buf),
                            DIR_MERGE_BOTH, basename(reports[curr]));
                else if (merge_runs)
                    sprintf(new_rep, "%s/%s/%s", dirname(buf),
                            DIR_MERGE_RUNS, basename(reports[curr]));
                else
                    sprintf(new_rep, "%s/%s/%s", dirname(buf),
                            DIR_MERGE_ALL, basename(reports[curr]));
                
                if (merged > 1) printf("into "YELLOW("%s")"\n", new_rep);
                save_report(&ap_rep[0], new_rep);
                reports_post[size_post++] = new_rep;
                
                merged = 0;
                free(reports[curr]);
            
            } else {
                
                ASSERT(0, "Impossible");
                
            }
            
            reports[curr] = NULL;
            curr = -1;
        } 
        
        i = (i + 1) % *size;
    }
    
    
    i = 0;
    while (i < *size) {
        if (reports_post[i] != NULL) 
            VG_(free)(reports[i]);
        i++;
    }
    VG_(free)(reports);
    
    *size = size_post;
    return reports_post;
}

static HChar ** merge_by_thread(HChar ** reports, UInt * size) {
    
    printf("Merging reports with same PID...\n");
    if (*size == 0) return reports;
    
    UInt size_post = 0;
    HChar ** reports_post = VG_(calloc)("test", sizeof(HChar *), *size);
    
    /*
     * Merge by PID
     */
    
    HChar merge_dir[1024] = {0};
    HChar buf[1024] = {0};
    STR(buf, "%s", reports[0]);
    int res = sprintf(merge_dir, "%s/%s", dirname(buf),
                        DIR_MERGE_THREAD);
    ASSERT(res < 1024, "path too long");
    res = mkdir(merge_dir, 0777);
    ASSERT(errno != EEXIST, "directory %s already exists", merge_dir);
    ASSERT(res == 0, "Invalid merge directory");
    
    UInt i = 0;
    Int curr = -1; UInt merged = 0;
    UInt curr_pid = 0;
    while (1) {
        
        if (reports[i] == NULL) goto next;
        
        //printf("checking report: %s\n", reports[i]);
        
        if (curr == -1) {
            
            curr = i;
            curr_pid = get_pid_report(reports[i]);
            
        } else {
            
            //printf("Current: %s\n", reports[curr]);
            if (curr_pid == get_pid_report(reports[i])) {
                
                if (merged == 0) {
                    reset_data(&ap_rep[0]);
                    printf("Merging %s ", reports[curr]);
                    Bool res = merge_report(reports[curr], &ap_rep[0]);
                    ASSERT(res, "Report %s should be merged but is invalid", 
                                        reports[curr]);
                    merged++;
                }

                printf("%s ", reports[i]);
                Bool res = merge_report(reports[i], &ap_rep[0]);
                ASSERT(res, "Report %s should be merged but is invalid", 
                            reports[i]);
                free(reports[i]);
                reports[i] = NULL;
                merged++;
            }
            
        }

next:            
        if (i + 1 == *size) {
            
            if (curr == -1) break;
            
            HChar * new_rep = VG_(calloc)("t", 1024, 2);
            HChar cmd[1024] = {0};
            if (merged > 0) {
                
                HChar buf[1024] = {0};
                STR(buf, "%s", reports[curr]);
                sprintf(new_rep, "%s/%s/%s", dirname(buf),
                            DIR_MERGE_THREAD, basename(reports[curr]));
                
                printf("into "YELLOW("%s")"\n", new_rep);
                //printf("saving %s to %s\n", reports[curr], new_rep);
                
                save_report(&ap_rep[0], new_rep);
                reports_post[size_post++] = new_rep;
                
                merged = 0;
            
            } else {
                
                HChar buf[1024] = {0};
                STR(buf, "%s", reports[curr]);
                sprintf(new_rep, "%s/%s/%s", dirname(buf),
                            DIR_MERGE_THREAD, basename(reports[curr]));
                
                //printf("no candidate for merging %s, copying to %s\n", reports[curr], new_rep);
                
                reports_post[size_post++] = new_rep;
                
                sprintf(cmd, "cp %s %s", reports[curr], new_rep);
                //printf("%s\n", cmd);
                res = system(cmd);
                ASSERT(res != -1, "Error during copy");
                
            }
            
            free(reports[curr]);
            reports[curr] = NULL;
            curr = -1;
        } 
        
        i = (i + 1) % *size;
    }
    
    i = 0;
    while (i < *size) {
        if (reports_post[i] != NULL) 
            VG_(free)(reports[i]);
        i++;
    }
    VG_(free)(reports);
    
    *size = size_post;
    return reports_post;
}

static void cmd_options(HChar * binary_name) {
    
    printf("usage: %s <action> [<action>] [<options>]\n", binary_name);
    
    printf("\n  Actions:\n");
    
    printf("    -k         check consistency of report(s)\n");
    
    printf("    -r         merge reports of different program's runs (different PID)\n");
    printf("               if no -t option is specified then this will not\n");
    printf("               merge reports of different threads.\n");
    
    printf("    -t         merge reports of different threads of the same program's run (same PID)\n");
    
    printf("    -i         merge reports (no criteria)\n");
    
    printf("    -c         compare two reports; required -a and -b arguments.\n");
    
    printf("\n  Other options:\n");
    
    printf("    -d <PATH>  report's directory [default: working directory]\n");
    printf("    -a <PATH>  perform an action on a specific report\n");
    printf("    -b <PATH>  compare report specified with -a with this report\n");
    
    printf("\n");
    return;
}

Int main(Int argc, HChar *argv[]) {

    //printf("aprof-helper - http://code.google.com/p/aprof/\n\n");
    
    /*
     * Parse options, sanitize options, etc
     */
    
    if (argc == 1) {
        cmd_options(argv[0]);
        return 1;
    }

    opterr = 0;
    Int opt;
    UInt i;
     
    while ((opt = getopt(argc, argv, "rtkcia:b:d:")) != -1) {
        
        switch(opt) {
            
            case 'i':
                merge_all = True;
                break;
            
            case 'a':
                logs[0] = optarg;
                if (optarg[0] == '=') logs[0]++;
                break;
                
            case 'b':
                logs[1] = optarg;
                if (optarg[0] == '=') logs[1]++;
                break;
                
            case 'c':
                compare = True;
                break;
                
            case 'k':
                consistency = True;
                break;
            
            case 'r':
                merge_runs = True;
                //printf("merge runs := True (%u)\n", merge_runs);
                break;
                
            case 't':
                merge_threads = True;
                //printf("merge threads := True (%u)\n", merge_threads);
                break;
                
            case 'd':
                if (optarg[0] == '=') directory++;
                ASSERT(strlen(optarg) > 0 && strlen(optarg) < 1024, 
                            "path too long");
                HChar dir[1024];
                if (optarg[strlen(optarg) -1] == '/') {
                    STR(dir, "%s", optarg);
                } else {
                    STR(dir, "%s/", optarg);
                }
                directory = dir;
                //printf("directory := %s\n", directory);
                break;
                
            case '?':
                if (optopt == 'd')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, 
                            "Unknown option character `\\x%x'.\n",
                            optopt);
            
                return 1;
                
            default:
                abort();
                
        }
        
    }
    
    if (!merge_all && !merge_runs && !merge_threads 
                            && !consistency && !compare) {
        cmd_options(argv[0]);
        return 1;
    }
    
    if (logs[0] != NULL && 
            strcmp(".aprof", logs[0] + strlen(logs[0]) - 6) != 0) {
        
        EMSG("Invalid first report\n");
        return 1;
    }
    
    if (logs[1] != NULL && 
            strcmp(".aprof", logs[1] + strlen(logs[1]) - 6) != 0) {
        
        EMSG("Invalid second report\n");
        return 1;
    }
    
    // compare (log, log)
    if (compare && (logs[0] == NULL || logs[1] == NULL)) {
        
        EMSG("compare option requires two log files\n");
        cmd_options(argv[0]);
        return 1;
    }
    
    if ((logs[0] != NULL || logs[1] != NULL) && directory != NULL) {
        
        EMSG("Please pass one/two log files or a directory, not both\n");
        cmd_options(argv[0]);
        return 1;
    }
    
    if (merge_runs || merge_threads || compare || merge_all) {
        consistency = False;
    }
    
    if ((merge_runs || merge_threads || merge_all) && compare) {
        
        EMSG("Too many actions\n");
        cmd_options(argv[0]);
        return 1;
    }
    
    if (merge_all && (merge_runs || merge_threads)) {
        
        EMSG("-i conflicts with -t or -r\n");
        cmd_options(argv[0]);
        return 1;
    }
    
    if (directory == NULL) {
        
        HChar cwd[1024];
        sprintf(cwd, "./");
        directory = cwd;
    
    }
    
    /*
    printf("Actions: ");
    if (compare) 
        printf("compare (consistency)\n");
    else if (merge_threads && merge_runs) 
        printf("merge_threads merge_runs (consistency)\n");
    else if (merge_threads)
        printf("merge_threads (consistency)\n");
    else if (merge_runs) 
        printf("merge_runs (consistency)\n");
    else if (consistency)
        printf("consistency\n");
    */
    
    UInt size = 0; 
    HChar ** reports = NULL;
    if (directory != NULL && logs[0] == NULL) {
        
        //printf("Searching reports in: %s\n", directory);
        reports = search_reports(&size);
    
    } else {
        
        reports = logs;
        //printf("Report[0]: %s\n", logs[0]);
        
        if (logs[1] == NULL) size = 1;
        else {
            //printf("Report[1]: %s\n", logs[1]);
            size = 2;
        }
        
    }

    /*
     * Do something!
     */
    
    if (consistency) {
        
        i = 0;
        while (i < size) {
            reset_data(&ap_rep[0]);
            merge_report(reports[i++], &ap_rep[0]);
        }
        
        printf(GREEN("Passed:")" ");
        if (size == 1)
            printf("Checked consistency of report: %s\n", reports[0]);
        else if (logs[0] != NULL && logs[1] != NULL) {
            printf("Checked consistency of report: %s\n", reports[0]);
            printf("Checked consistency of report: %s\n", reports[1]);
        } else
            printf("Checked consistency of %d reports in: %s\n", 
                        size, dirname(reports[0]));
    
    } else if (compare) {
        
        reset_data(&ap_rep[0]);
        merge_report(reports[0], &ap_rep[0]);
        reset_data(&ap_rep[1]);
        merge_report(reports[1], &ap_rep[1]);
        compare_report(&ap_rep[0], &ap_rep[1], STRICT);
        
    } else { // merge
        
        if (merge_threads)
            reports = merge_by_thread(reports, &size);
        
        if (merge_all)
            ASSERT(!merge_threads, "Invalid");
        
        if (merge_runs || merge_all)
            reports = merge_by_run(reports, &size, merge_threads);

    }
    
    /*
     * Clean up memory
     */
     
    for (i = 0; i < SLOT; i++)
        clean_data(&ap_rep[i]);
        
    if (directory != NULL && logs[0] == NULL) {
    
        i = 0;
        while (i < size) {
            if (reports[i] != NULL) 
                VG_(free)(reports[i]);
            i++;
        }
        VG_(free)(reports);
    
    }
    
    return 0;
}
