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

#define DEBUG 1

#define DELIM_SQ '$'
#define DELIM_DQ "$"

#define EMSG(...) fprintf(stderr, __VA_ARGS__);

#define ASSERT(cond, ...)   do { \
                                    if (!(cond)) { \
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

#define BG_GREEN   "\x1b[42m"
#define BG_RESET   "\x1b[0m"
#define BG_RED     "\x1b[41m"

#define GREEN(str) BG_GREEN str BG_RESET
#define RED(str) BG_RED str BG_RESET

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

Bool consistency = False;
Bool compare = False;
Bool merge_runs = False;
Bool merge_threads = False;
HChar * directory = NULL;
HChar * logs[SLOT] = {NULL, NULL}; // only for compare

typedef struct aprof_report {
    
    UInt version;
    UInt input_metric;
    HashTable * fn_ht;
    HashTable * obj_ht;
    HashTable * routine_hash_table;
    
    ULong performance_metric;
    ULong real_total_cost;
    ULong self_total_cost;

    ULong next_routine_id;
    HChar * cmd;
    
    ULong tmp;

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
    HT_destruct(ri->rms_map);
    HT_destruct(ri->distinct_rms);
    
    VG_(free)(rtn);
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
    rtn_info->rms_map = HT_construct(free);
    DASSERT(rtn_info->rms_map != NULL, "rms_map not allocable");
    
    rtn_info->distinct_rms = HT_construct(free);
    DASSERT(rtn_info->distinct_rms != NULL, "rms_map not allocable");
    
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
                                    UInt * rid) {
    
    if (VG_(strlen)(line_input) <= 0) return curr;
    HChar * line = put_delim(line_input);
    
    ASSERT(VG_(strlen)(line_input) + VG_(strlen)(report) + 1 < 2048,
        "line + report too long");
    HChar * line_orig = VG_(calloc)("report line", 2048, 1);
    line_orig = VG_(strcat)(line_orig, "\n\tReport: ");
    line_orig = VG_(strcat)(line_orig, report);
    line_orig = VG_(strcat)(line_orig, "\n\tLine: ");
    line_orig = VG_(strcat)(line_orig, line_input);
    line_orig = VG_(strcat)(line_orig, "\n");
    
    HChar * token = VG_(strtok)(line, DELIM_DQ);
    ASSERT(token != NULL, "Invalid line: %s", line_orig); 
    
    if (token[0] == 'r') {
        
        if (curr != NULL) 
            ASSERT(r->tmp > 0, "Invalid cumul: %s", line_orig);
        
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
        
        if (min <= 100000)
            ASSERT(sqr_sum >= ((double)min)*((double)min)*
                    ((double)occ), "Invalid sqr_sum: %s", line_orig);
        
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
        ASSERT(self_sqr >= ((double)self_min) * ((double)self_min)
                    * ((double)occ), "Invalid self sqr: %s", line_orig);
        
        ULong rms_sum = 0;
        double rms_sqr = 0;
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

        }

        ASSERT(curr != NULL, "Invalid routine: %s", line_orig);
        RMSInfo * info_access = HT_lookup(curr->rms_map, rms); 
        if (info_access == NULL) {
            
            info_access = (RMSInfo * ) VG_(calloc)("rms_info", 1, sizeof(RMSInfo));
            DASSERT(info_access != NULL, "rms_info not allocable in function exit");
            
            info_access->min_cumulative_time = min;
            info_access->self_time_min = self_min;
            info_access->key = rms;
            HT_add_node(curr->rms_map, info_access->key, info_access);
            
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
        if (info_access->min_cumulative_time <= 100000)
            ASSERT(info_access->cumulative_sum_sqr >= 
                ((double)info_access->min_cumulative_time) * 
                ((double)info_access->min_cumulative_time) *
                ((double)info_access->calls_number), 
                "Invalid sqr_sum");
        
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
        ASSERT(info_access->self_sum_sqr >= 
                ((double)info_access->self_time_min) * 
                ((double)info_access->self_time_min) *
                ((double)info_access->calls_number), 
                "Invalid sqr_sum");
        
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
        
        }
    
    } else if (token[0] == 'g') {
    
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        DASSERT(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(id, line_orig);
        
        ASSERT(*rid == id, "Routine id mismatch: %s", line_orig);
        
        // RMS
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid rms: %s", line_orig);
        ULong rms = VG_(strtoull10) (token, NULL);
        UOF_LONG(rms, line_orig);
        
        ASSERT(curr != NULL, "distinct rms for a void routine: %s", line_orig);
        HashNode * node = HT_lookup(curr->distinct_rms, rms);
        if (node == NULL) {
            
            node = VG_(calloc)("distinct rms node", sizeof(HashNode), 1);
            node->key = rms;
            HT_add_node(curr->distinct_rms, node->key, node);
        
        }

    } else if (token[0] == 'a') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL && VG_(strlen)(token) > 0, 
            "invalid app name: %s", line_orig); 
        HChar app[1024] = { 0 };
        while (token != NULL) {
            
            ASSERT(VG_(strlen)(app) + VG_(strlen)(token) + 1 < 1024,
                "app name too long: %s", line_orig); 
            
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
            
            ASSERT(VG_(strcmp)(app, r->cmd) == 0, 
                "different command");
            
        }
    
    } else if (token[0] == 'k') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid perf metric: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(sum, line_orig);
        ASSERT(sum > 0, "Invalid sum: %s", line_orig);
        
        ADD(r->performance_metric, sum);
        
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
                
                ASSERT(r->version == ver, 
                "You are elaborating reports of different versions: %s", 
                line_orig);
            
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
            
            if (VG_(strcmp)("rms", token) == 0)
                r->input_metric = RMS;
            else if (VG_(strcmp)("rvms", token) == 0)
                r->input_metric = RVMS;
            else
                ASSERT(0, "Invalid input metric: %s", line_orig);
        
        } else if (r->version == REPORT_VERSION_OLD)
            r->input_metric = RMS;
        else
            ASSERT(0, "Invalid version: %s", line_orig);
    
    } else if (token[0] == 'c' || token[0] == 'e' || token[0] == 'm'
                    || token[0] == 't' || token[0] == 'f'  
                    || token[0] == 'u'){ 
    
        // ignored...
    
    } else {
        
        ASSERT(0, "Unknown tag %c %s", token[0], line_orig);
        
    }
    
    VG_(free)(line);
    VG_(free)(line_orig);
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
        double sum_rms = 0;
        double sum_rvms = 0;
        
        HT_ResetIter(rtn->rms_map);
        RMSInfo * i = (RMSInfo *) HT_Next(rtn->rms_map);
        while (i != NULL) {
            
            if (r->version == REPORT_VERSION) {
                ADD(sum_rms, i->rms_input_sum);
                ADD(sum_rvms, i->key*i->calls_number);
            }
            
            ADD(cumul_real, i->cumul_real_time_sum);
            i = (RMSInfo *) HT_Next(rtn->rms_map);
        }
        
        ASSERT(cumul_real > 0, "Invalid cumul real: %s:%s", 
                    rtn->fn->name, report);
        
        if (r->version == REPORT_VERSION)
            check_rvms(sum_rms, sum_rvms, 
                        HT_count_nodes(rtn->distinct_rms),
                        HT_count_nodes(rtn->rms_map),
                        rtn->fn->name, report);
        
        rtn = (RoutineInfo *) HT_Next(r->routine_hash_table);
    }
    
}

static Bool merge_report(HChar * report, aprof_report * rep_data) {
    
    //VG_(printf)("Try to merge: %s\n", report);
    HChar buf[4096];
    
    if (directory != NULL) {
        STR(buf, "%s/%s", directory, report);
    } else {
        STR(buf, "%s", report);
    }
    
    Int file = VG_(open)(buf, O_RDONLY, S_IRUSR|S_IWUSR);
    ASSERT(file >= 0, "Can't read: %s", buf);
    
    HChar line[1024];
    UInt offset = 0;
    RoutineInfo * current_routine = NULL;
    UInt curr_rid = 0;
    
    /* merge tuples */
    while (1) {
        
        Int r = VG_(read)(file, buf, 4096);
        if (r < 0) {
            
            EMSG("Error when reading %s\n", report);
            return False;
        
        } else if (r == 0) return True; /* EOF */
        
        Int i;
        for (i = 0; i < r; i++) {
            
            if (buf[i] == '\n') {
                
                line[offset++] = '\0';
                current_routine = merge_tuple(line, current_routine, 
                                                report, rep_data, &curr_rid);
                offset = 0;
            
            } else line[offset++] = buf[i]; 
            
            ASSERT(offset < 1024, "Line too long");
        }
        
    }    
    
    line[offset++] = '\0';
    current_routine = merge_tuple(line, current_routine, report, 
                                        rep_data, &curr_rid);

    VG_(close)(file);
    
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
            
            if (*n + 1 == size) {
                size = size * 2;
                reports = realloc(reports, sizeof(HChar *) * size);
            }
            reports[(*n)++] = VG_(strdup2)("report", (HChar *)file->d_name);
            
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

static void compare_distinct_rms(RoutineInfo * rtn, RoutineInfo * rtn2,
                                    aprof_report * r, aprof_report * r2,
                                    UInt mode) {
    
    //HChar b[1024];
    return;
}

static int compar(const void * a, const void * b) {
    
    ULong c = *((ULong *) a);
    ULong d = *((ULong *) b);
    
    if (c == d) return 0;
    if (c < d) return -1;
    else return 1;
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
    if (HT_count_nodes(rtn->rms_map) != HT_count_nodes(rtn2->rms_map)) {
    
        STR(b, "# RVMS [%s]", rtn->fn->name);
        printf("%-*s", STR_ALIGN, b);
        print_diff(HT_count_nodes(rtn->rms_map), HT_count_nodes(rtn2->rms_map));
        
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
    
    ULong * rms_1 = malloc(HT_count_nodes(rtn->rms_map) * sizeof(ULong));
    ULong * rms_2 = malloc(HT_count_nodes(rtn2->rms_map) * sizeof(ULong));

    // Collect RMS of rtn
    HT_ResetIter(rtn->rms_map);
    RMSInfo * i = NULL;
    while (1) {
        
        i = (RMSInfo *) HT_Next(rtn->rms_map);
        if (i == NULL) break;
        
        rms_1[k++] = i->key;
    }
    
    // Collect RMS of rtn2
    HT_ResetIter(rtn2->rms_map);
    i = NULL; k = 0;
    while (1) {
        
        i = (RMSInfo *) HT_Next(rtn2->rms_map);
        if (i == NULL) break;
        
        rms_2[k++] = i->key;
    }
    
    // sort
    qsort(rms_1, HT_count_nodes(rtn->rms_map), sizeof(ULong), compar);
    qsort(rms_2, HT_count_nodes(rtn2->rms_map), sizeof(ULong), compar);

    ULong call = 0;
    ULong call2 = 0;
    for (k = 0, j = 0; k < HT_count_nodes(rtn->rms_map); k++) {
        
        i = (RMSInfo *) HT_lookup(rtn->rms_map, rms_1[k]);
        if (i == NULL) ASSERT(0, "Impossible");
        
        ADD(sum_rvms, i->key * i->calls_number);
        ADD(calls, i->calls_number);
        ADD(sum_real_cost, i->cumul_real_time_sum);
        ADD(sum_self_cost, i->self_time_sum);
        ADD(sum_rms, i->rms_input_sum);
        
        RMSInfo * i2 = NULL;
        if (j < HT_count_nodes(rtn2->rms_map))
            i2 = (RMSInfo *) HT_lookup(rtn2->rms_map, rms_2[j]);
       
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
    for (k = 0, j = 0; k < HT_count_nodes(rtn2->rms_map); k++) {
        
        i = (RMSInfo *) HT_lookup(rtn2->rms_map, rms_2[k]);
        if (i == NULL) ASSERT(0, "Impossible");
        
        ADD(sum_rvms2, i->key * i->calls_number);
        ADD(calls2, i->calls_number);
        ADD(sum_real_cost2, i->cumul_real_time_sum);
        ADD(sum_self_cost2, i->self_time_sum);
        ADD(sum_rms2, i->rms_input_sum);
        
        RMSInfo * i2 = NULL;
        if (j < HT_count_nodes(rtn->rms_map))
            i2 = (RMSInfo *) HT_lookup(rtn->rms_map, rms_1[j]);
        
        if (i2 == NULL) {

            IFS(1) {
                
                STR(b, "Missing RVMS [%s]", rtn->fn->name);
                printf("%-*s", STR_ALIGN, b);
                print_diff(0, i->key);
            
            }
        
        } else if (i->key != i2->key) {
            
            // already handled
        
        }
        
        ADD(call, i->calls_number);
        if (i2 != NULL && call >= call2 + i2->calls_number) {
            ADD(call2, i2->calls_number);
            j++;
        }
        
    }

    /* this is not so useful
    STR(b, "Diff sum(RVMS) [%s]", rtn->fn->name);
    print_diff_if_input(sum_rvms, sum_rvms2, b);    
    */
    
    if (True) {
        
        STR(b, "Diff sum(calls) [%s]", rtn->fn->name);
        print_diff_if_cost(calls, calls2, b); 

        STR(b, "Diff sum(real_cost) [%s]", rtn->fn->name);
        print_diff_if_cost(sum_real_cost, sum_real_cost2, b);

        STR(b, "Diff sum(self_cost) [%s]", rtn->fn->name);
        print_diff_if_cost(sum_self_cost, sum_self_cost2, b);
    
    }
    
    /* this is not so useful
    STR(b, "Diff sum(rms) [%s]", rtn->fn->name);
    print_diff_if_input(sum_rms, sum_rms2, b);
    */

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

static void clean_data(aprof_report * rep) {
    
    ASSERT(rep != NULL, "Invalid aprof report");
    
    if (rep->fn_ht != NULL) HT_destruct(rep->fn_ht);
    if (rep->obj_ht != NULL) HT_destruct(rep->obj_ht);
    if (rep->routine_hash_table != NULL) HT_destruct(rep->routine_hash_table);
    if (rep->cmd != NULL) VG_(free)(rep->cmd);
    
}

static void reset_data(aprof_report * rep) {
    
    clean_data(rep);
    
    rep->next_routine_id = 1;
    rep->cmd = NULL;
    rep->fn_ht = HT_construct(fn_destroy);
    rep->obj_ht = HT_construct(obj_destroy);
    rep->routine_hash_table = HT_construct(destroy_routine_info);
    rep->version = 0;
    rep->input_metric = 0;
    
    return;
    
}

static void cmd_options(HChar * binary_name) {
    
    printf("usage: %s <action> [<action>] [<options>]\n", binary_name);
    
    printf("\n  Actions:\n");
    
    printf("    -r         Merge reports of different program's runs\n");
    printf("               if no -t option is specified then this will not\n");
    printf("               merge reports of different threads.\n");
    
    printf("    -t         Merge reports of different threads of the same program's run\n");
    
    printf("\n  Other options:\n");
    
    printf("    -d <PATH>  report's directory [default: working directory]\n");
    
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
     
    while ((opt = getopt(argc, argv, "rtkca:b:d:")) != -1) {
        
        switch(opt) {
            
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
                directory = optarg;
                if (optarg[0] == '=') directory++;
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
    
    if (!merge_runs && !merge_threads && !consistency && !compare) {
        cmd_options(argv[0]);
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
    
    if (merge_runs || merge_threads || compare) {
        consistency = False;
    }
    
    if ((merge_runs || merge_threads) && compare) {
        
        EMSG("Too many actions\n");
        cmd_options(argv[0]);
        return 1;
    }
    
    if (directory == NULL) {
        
        HChar cwd[1024];
        directory = getcwd(cwd, 1024);
    
    }
    
    UInt size = 0; 
    HChar ** reports = NULL;
    if (directory != NULL && logs[0] == NULL) {
        
        printf("Searching reports in: %s\n", directory);
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
        printf("Checked consistency of %u reports\n", size);
    
    } else if (compare) {
        
        reset_data(&ap_rep[0]);
        merge_report(reports[0], &ap_rep[0]);
        reset_data(&ap_rep[1]);
        merge_report(reports[1], &ap_rep[1]);
        compare_report(&ap_rep[0], &ap_rep[1], STRICT);
        
    } else { // merge
        
        
        
    }
    
    /*
     * Clean up memory
     */
     
    for (i = 0; i < SLOT; i++)
        clean_data(&ap_rep[i]);
        
    if (directory != NULL && logs[0] == NULL) {
    
        i = 0;
        while (i < size) VG_(free)(reports[i++]);
        VG_(free)(reports);
    
    }
    
    return 0;
}
