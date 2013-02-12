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

#define DEBUG 1

#define DELIM_SQ '$'
#define DELIM_DQ "$"

#define EMSG(...) fprintf(stderr, __VA_ARGS__);

#define ASSERT(cond, ...)   do { \
                                    if (!(cond)) { \
                                        EMSG(__VA_ARGS__); \
                                        assert(cond); \
                                    } \
                            } while(0);

// check for overflow
#define ADD(dest, inc) do { \
                            ULong old = dest; \
                            dest += inc; \
                            ASSERT(dest >= old, "overflow"); \
                            } while(0); 

// check under/overflow strtol
#define UOF_LONG(val, line) ASSERT(val != LLONG_MIN && val != LLONG_MAX, \
                            "under/over flow: %s", line);

// Debug assert
#if DEBUG
#define DASSERT(cond, ...) ASSERT(cond, __VA_ARGS__)
#else // DEBUG
#define DASSERT(cond, ...) // do nothing
#endif // DEBUG

#define SLOT 2

// Sum performance metric (# BB) btw all reports merged
static ULong sum_performance_metric[SLOT] = {0, 0};

// next routine ID
static ULong next_routine_id[SLOT] = {1, 1};

// command
HChar * report_cmd[SLOT] = {NULL, NULL};

Bool consistency = False;
Bool compare = False;
Bool merge_runs = False;
Bool merge_threads = False;
HChar * directory = NULL;
HChar * logs[SLOT] = {NULL, NULL};
UInt version[SLOT] = {0, 0};

static HashTable * fn_ht[SLOT] = {NULL, NULL};
static HashTable * obj_ht[SLOT] = {NULL, NULL};
static HashTable * routine_hash_table[SLOT] = {NULL, NULL};

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

static RoutineInfo * new_routine_info(Function * fn, UWord target, UInt slot) {
    
    ASSERT(slot < SLOT, "Invalid slot");
    DASSERT(fn != NULL, "Invalid function info");
    DASSERT(target > 0, "Invalid target");
    
    RoutineInfo * rtn_info = VG_(calloc)("rtn_info", 1, sizeof(RoutineInfo));
    DASSERT(rtn_info != NULL, "rtn info not allocable");
    
    rtn_info->key = target;
    rtn_info->fn = fn;
    rtn_info->routine_id = next_routine_id[slot]++;
    rtn_info->distinct_rms = 0;

    /* elements of this ht are freed when we generate the report */
    rtn_info->rms_map = HT_construct(free);
    DASSERT(rtn_info->rms_map != NULL, "rms_map not allocable");
    
    HT_add_node(routine_hash_table[slot], rtn_info->key, rtn_info);
    
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
                                    HChar * report, UInt slot, UInt * rid) {
    
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
        Function * fn = HT_lookup(fn_ht[slot], hash);     
        while (fn != NULL && VG_(strcmp)(fn->name, name) != 0) {
            
            fn = fn->next;
        }
        
        if (fn == NULL) { // this is a new function
            
            fn = VG_(calloc)("fn", sizeof(Function), 1);
            DASSERT(fn != NULL, "Function not allocable");
            
            fn->key = hash;
            fn->name = name;
            
            HT_add_node(fn_ht[slot], fn->key, fn);
            
        } else VG_(free)(name);
        
        if (fn->obj == NULL) { // new object
            
            UInt hash_obj = str_hash(obj_name);
            Object * obj = HT_lookup(obj_ht[slot], hash_obj);
            while (obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0) {
                
                obj = obj->next;
            }
            
            if (obj == NULL) {
 
                obj = VG_(calloc)("obj", sizeof(Object), 1);
                DASSERT(obj != NULL, "Obj not allocable");
                
                obj->key = hash_obj;
                obj->name = obj_name;
                obj->filename = NULL; /* FixMe */
                HT_add_node(obj_ht[slot], obj->key, obj);
                
                fn->obj = obj;
            
            } else VG_(free)(obj_name);
            
            fn->obj = obj;
        
        } else VG_(free)(obj_name);
        
        curr = HT_lookup(routine_hash_table[slot], (UWord) fn);
        if (curr == NULL) {
            
            curr = new_routine_info(fn, (UWord) fn, slot);
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
        ULong sqr_sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(sqr_sum, line_orig);
        ASSERT(sqr_sum >= sum, "Invalid sqr_sum: %s", line_orig);
        
        // occ
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid occ: %s", line_orig);
        ULong occ = VG_(strtoull10) (token, NULL);
        UOF_LONG(occ, line_orig);
        ASSERT(occ > 0, "Invalid occ: %s", line_orig);
        
        ASSERT(sum >= min*occ, "Invalid sum: %s", line_orig);
        ASSERT(sqr_sum >= min*min*occ, "Invalid sqr_sum: %s", line_orig);
        
        // cumul_real
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid cumul: %s", line_orig);
        ULong cumul_real = VG_(strtoull10) (token, NULL);
        UOF_LONG(cumul_real, line_orig);
        ASSERT(cumul_real >= 0 && cumul_real <= sum, 
                    "Invalid cumul: %s", line_orig);
        
        // self_total
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self total: %s", line_orig);
        ULong self = VG_(strtoull10) (token, NULL);
        UOF_LONG(self, line_orig);
        ASSERT(self > 0, "Invalid self total: %s", line_orig);
        
        // self_min
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self min: %s", line_orig);
        ULong self_min = VG_(strtoull10) (token, NULL);
        UOF_LONG(self_min, line_orig);
        ASSERT(self_min > 0, "Invalid self min: %s", line_orig);
        
        ASSERT(self >= self_min*occ, "Invalid self total: %s", line_orig);
        
        // self_max
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid self max: %s", line_orig);
        ULong self_max = VG_(strtoull10) (token, NULL);
        UOF_LONG(self_max, line_orig);
        ASSERT(self_max >= self_min, "Invalid self max: %s", line_orig);
        
        // sqr self
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid sqr self: %s", line_orig);
        ULong self_sqr = VG_(strtoull10) (token, NULL);
        UOF_LONG(self_sqr, line_orig);
        ASSERT(self_sqr >= self_min*self_min*occ, 
                        "Invalid self sqr: %s", line_orig);
        
        
        ULong rms_sum = 0;
        ULong rms_sqr = 0;
        if (version[slot] == REPORT_VERSION) {
        
            #if INPUT_METRIC == RVMS
            // rms sum
            token = VG_(strtok)(NULL, DELIM_DQ);
            ASSERT(token != NULL, "Invalid rms sum: %s", line_orig);
            rms_sum = VG_(strtoull10) (token, NULL);
            UOF_LONG(rms_sum, line_orig);
            ASSERT(rms_sum <= rms*occ, "invalid rms sum: %s", line_orig);
            
            // ratio sum sqr
            token = VG_(strtok)(NULL, DELIM_DQ);
            ASSERT(token != NULL, "Invalid sqr rms: %s", line_orig);
            rms_sqr = VG_(strtoull10) (token, NULL);
            UOF_LONG(rms_sqr, line_orig);
            ASSERT(rms_sqr <= rms*rms*occ, "invalid rms sqr: %s", line_orig);
            #endif // INPUT_METRIC == RVMS

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
        ADD(info_access->cumulative_sum_sqr, sqr_sum);
        ADD(info_access->calls_number, occ);

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
        ASSERT(info_access->cumulative_sum_sqr >= 
                info_access->min_cumulative_time * 
                info_access->min_cumulative_time *
                info_access->calls_number, 
                "Invalid sqr_sum");
        
        ADD(info_access->cumul_real_time_sum, cumul_real);
        ADD(info_access->self_time_sum, self);
        ADD(info_access->self_sum_sqr, self_sqr);
        
        ASSERT(info_access->cumul_real_time_sum >= 0 
                    && info_access->cumul_real_time_sum <= 
                    info_access->cumulative_time_sum, 
                    "Invalid cumul: %s", line_orig);
        
        if (info_access->self_time_min > self_min) 
            info_access->self_time_min = self_min;
    
        if (info_access->self_time_max < self_max) 
            info_access->self_time_max = self_max;
        
        ASSERT(info_access->self_time_max >= 
                info_access->self_time_min, "Invalid self min/max");
        ASSERT(info_access->self_time_sum >= 
                info_access->self_time_max, "Invalid self sum");
        
        ASSERT(info_access->self_time_sum >= 
                info_access->self_time_min *
                info_access->calls_number, 
                "Invalid sum");
        ASSERT(info_access->self_sum_sqr >= 
                info_access->self_time_min * 
                info_access->self_time_min *
                info_access->calls_number, 
                "Invalid sqr_sum");
        
        if (version[slot] == REPORT_VERSION) {
            
            #if INPUT_METRIC == RVMS
            ADD(info_access->rms_input_sum, rms_sum);
            ADD(info_access->rms_input_sum_sqr, rms_sqr);
            
            ASSERT(info_access->rms_input_sum <= 
                        rms * info_access->calls_number, 
                        "invalid rms sum");
            
            ASSERT(info_access->rms_input_sum_sqr <= 
                        rms * rms * info_access->calls_number, 
                        "invalid rms sqr");
            
            #endif // INPUT_METRIC == RVMS
        
        }
    
    } else if (token[0] == 'g') {
    
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, NULL); 
        DASSERT(id > 0, "Invalid ID: %s", line_orig);
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
        
        if (report_cmd[slot] == NULL) {
            
            HChar * app_c = VG_(strdup2)("cmd", app);
            report_cmd[slot] = app_c;
        
        } else {
            
            ASSERT(VG_(strcmp)(app, report_cmd[slot]) == 0, 
                "different command");
            
        }
    
    } else if (token[0] == 'k') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid perf metric: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(sum, line_orig);
        ASSERT(sum > 0, "Invalid sum: %s", line_orig);
        
        ADD(sum_performance_metric[slot], sum);
        
    } else if (token[0] == 'v') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        ASSERT(token != NULL, "Invalid version: %s", line_orig);
        ULong ver = VG_(strtoull10) (token, NULL);
        UOF_LONG(ver, line_orig);
        ASSERT(ver == REPORT_VERSION || ver == REPORT_VERSION_OLD, 
            "Invalid version: %s", line_orig);
        
        if (version[slot] == 0) version[slot] = ver;
        if (merge_runs || merge_threads || compare){
            
            if (slot > 0) {
            
                ASSERT(version[0] == version[1], 
                "You are elaborating reports of different versions: %s", 
                line_orig);
            
            } else {
                
                ASSERT(version[0] == ver, 
                "You are elaborating reports of different versions: %s", 
                line_orig);
            
            }
            
        }
        
    } else if (token[0] == 'q' || token[0] == 'x') {
        
        ASSERT(0, "Merge of reports with CCT is not yet supported");
        
    }
    
    VG_(free)(line);
    VG_(free)(line_orig);
    return curr;
}

static void post_merge_consistency(UInt slot, HChar * report) {
    
    HT_ResetIter(routine_hash_table[slot]);
    RoutineInfo * rtn = (RoutineInfo *) HT_Next(routine_hash_table[slot]);
    while (rtn != NULL) {
        
        ULong cumul_real = 0;
        
        HT_ResetIter(rtn->rms_map);
        RMSInfo * i = (RMSInfo *) HT_Next(rtn->rms_map);
        while (i != NULL) {
            
            ADD(cumul_real, i->cumul_real_time_sum);
            i = (RMSInfo *) HT_Next(rtn->rms_map);
        }
        
        ASSERT(cumul_real > 0, "Invalid cumul real");
        
        rtn = (RoutineInfo *) HT_Next(routine_hash_table[slot]);
    }
    
    
}

static Bool merge_report(HChar * report, UInt slot) {
    
    //VG_(printf)("Try to merge: %s\n", report);
    
    Int file = VG_(open)(report, O_RDONLY, S_IRUSR|S_IWUSR);
    ASSERT(file >= 0, "Can't read: %s", report);
    
    HChar buf[4096];
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
                                                report, slot, &curr_rid);
                offset = 0;
            
            } else line[offset++] = buf[i]; 
            
            ASSERT(offset < 1024, "Line too long");
        }
        
    }    
    
    line[offset++] = '\0';
    current_routine = merge_tuple(line, current_routine, report, 
                                        slot, &curr_rid);
    
    VG_(close)(file);
    
    post_merge_consistency(slot, report);
    
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

static void clean_data(UInt slot) {
    
    ASSERT(slot < SLOT, "Invalid slot");
    
    if (fn_ht[slot] != NULL) HT_destruct(fn_ht[slot]);
    if (obj_ht[slot] != NULL) HT_destruct(obj_ht[slot]);
    if (routine_hash_table[slot] != NULL) HT_destruct(routine_hash_table[slot]);
    if (report_cmd[slot] != NULL) VG_(free)(report_cmd[slot]);
    
}

static void reset_data(UInt slot) {
    
    clean_data(slot);
    
    next_routine_id[slot] = 1;
    report_cmd[slot] = NULL;
    fn_ht[slot] = HT_construct(fn_destroy);
    obj_ht[slot] = HT_construct(obj_destroy);
    routine_hash_table[slot] = HT_construct(destroy_routine_info);
    version[slot] = 0;
    
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
    
    printf("aprof-helper - http://code.google.com/p/aprof/\n\n");
    
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
    if (directory != NULL) {
        
        printf("Searching reports in: %s\n", directory);
        reports = search_reports(&size);
    
    } else {
        
        reports = logs;
        if (logs[1] == NULL) size = 1;
        else size = 2;
        
    }
    
    if (consistency) {
        
        i = 0;
        while (i < size) {
            reset_data(0);
            merge_report(reports[i++], 0);
        }
        printf("Checked consistency of %u reports\n", size);
    
    }
    
    
    // Clean up memory
    for (i = 0; i < SLOT; i++)
        clean_data(i);
    if (directory != NULL) {
    
        i = 0;
        while (i < size) VG_(free)(reports[i++]);
        VG_(free)(reports);
    
    }
    
    return 0;
}
