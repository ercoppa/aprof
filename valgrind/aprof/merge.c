/*
 * Merge report functions
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
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

#ifndef EXTERNAL
    #include "aprof.h"
#else
    #include "extra/aprof-helper.h"
    #include "data-common.h"
#endif

#define DELIM_SQ '$'
#define DELIM_DQ "$" 

#if !APROF_TOOL
    
    #define HELPER(action) action
    #define WARNING(cond, ...) do { if (!(cond)) EMSG(YELLOW("Warning: ") __VA_ARGS__) } while(0);
    #define UOF_LONG(error, token, val, line) APROF_(assert)(errno != ERANGE, \
                                                "under/over flow: %s", line);

    #define UOF_DOUBLE(error, token, val, line) APROF_(assert)(errno != ERANGE, \
                                                "under/over flow: %s", line);
    
#else
    #define HELPER(action)
    #define WARNING(cond, ...) do { if (!(cond)) { VG_(umsg)("Warning: " __VA_ARGS__); }} while(0);
    #define UOF_LONG(error, token, val, line) APROF_(assert)(!(val == 0 && error == token), \
                                                                "under/over flow: %s", line)
    #define UOF_DOUBLE(error, token, val, line) APROF_(assert)(!(val == 0 && error == token), \
                                                               "under/over flow: %s", line)
#endif

UInt APROF_(search_reports)(HChar *** reports) {

    SysRes r = VG_(open)("./", VKI_O_RDONLY, VKI_S_IRUSR|VKI_S_IWUSR);
    Int dir = (Int) sr_Res(r);
    if (dir < 0) return 0; 
    
    UInt size = 0;
    UInt capacity = 16;
    *reports = VG_(calloc)("report list", sizeof(HChar *), capacity);

    struct vki_dirent * file;
    HChar buf[1024] = {0};

    while(1) {

        Int res = VG_(getdents)(dir, (struct vki_dirent *) &buf, 1024);
        if (res <= 0) break;
        
        Int i;
        for(i = 0; i < res; i += file->d_reclen) {

            file = (struct vki_dirent *) (buf + i);
            
            // Invalid file name
            if (file->d_name == NULL) continue;
            
            // filename to short
            if (VG_(strlen)(file->d_name) <= 7) continue;
            
            // filename contains ".aprof"
            if (VG_(strcmp)(".aprof", file->d_name + 
                VG_(strlen)(file->d_name) - 6) == 0) {
                
                if (size + 1 == capacity) {
                    capacity *= 2;
                    *reports = VG_(realloc)("report list", *reports, 
                                            capacity * sizeof(HChar *)); 
                }
                
                *(*reports + size) = VG_(strdup)("report", file->d_name);;
                //VG_(printf)("File %s\n", *(*reports + size));
                size += 1;
            }            
        }
   }
   return size;
}

static Int APROF_(get_memory_resolution_report)(HChar * report) {
    
    HChar * rep = (HChar *) VG_(basename)(report);
    if (rep == NULL) return -1;
    
    // start from the end
    HChar * p = rep + VG_(strlen)(rep) - 1;
    
    // skip ".aprof"
    if (!(*p == 'f' && --p > rep)) return -1;
    if (!(*p == 'o' && --p > rep)) return -1;
    if (!(*p == 'r' && --p > rep)) return -1;
    if (!(*p == 'p' && --p > rep)) return -1;
    if (!(*p == 'a' && --p > rep)) return -1;
    if (!(*p == '.' && --p > rep)) return -1;
    
    // memory resolution
    while (p > rep && *p != '_') p--;
    if (!(p > rep)) return -1;
    
    p++;
    Int res = VG_(strtoull10)(p, NULL);
    
    return res;
}

static HChar * APROF_(put_delim)(HChar * str_orig) {
    
    Int size = VG_(strlen)(str_orig);    
    HChar * str = VG_(strdup)("str", str_orig);
    
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

static Function * APROF_(merge_tuple)(  HChar * line_input, 
                                        Function * curr,
                                        HChar * report, 
                                        Runtime * r,
                                        UInt * rid,
                                        Bool * invalid,
                                        ULong * total_cost) {
    
    //VG_(umsg)("Analyzing %s\n", line_input);
    
    HashTable * fn_ht = r->fn_ht;
    HashTable * obj_ht = r->obj_ht;
    //HChar * cmd = r->cmd_line;
    HChar * app = r->application;
    input_metric_t input_metric = r->input_metric;
    UInt memory_resolution = r->memory_resolution;
    ULong binary_mtime = r->binary_mtime;
    
    HChar * error;
    *invalid = False;
    
    if (VG_(strlen)(line_input) <= 0) return curr;
    HChar * line = APROF_(put_delim)(line_input);
    
    APROF_(assert)(VG_(strlen)(line_input) + VG_(strlen)(report) + 1 < 2048,
                        "line + report too long");
    HChar line_orig[2048] = { 0 };
    VG_(sprintf)(line_orig, "\n\tReport: %s\n\tLine: %s\n", report, line_input);
    
    HChar * token = VG_(strtok)(line, DELIM_DQ);
    APROF_(assert)(token != NULL, "Invalid line: %s", line_orig);
    
    if (token[0] == 'r') {
        
        if (curr == NULL) {
            
            // if we hit the first r tag then we are starting 
            // to merge the report. Then sum the value of "tag k"
            // saved previously in total_cost
            
            ADD(r->extra_cost, *total_cost);
        }
            
        
        // function name
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid fn name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        APROF_(assert)(VG_(strlen)(token) > 0, "Invalid fn name: %s", line_orig);
        HChar * name = VG_(strdup)("fn_name", token);
        
        // object name
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid obj name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        APROF_(assert)(VG_(strlen)(token) > 0, "Invalid obj name: %s", line_orig);
        HChar * obj_name = VG_(strdup)("obj_name", token);
        
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, &error);
        APROF_(assert)(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(error, token, id, line_orig);
        
        *rid = id;
        
        // Search function
        UInt hash = APROF_(str_hash)(name);
        Function * fn = HT_lookup(fn_ht, hash);
        while (fn != NULL && VG_(strcmp)(fn->name, name) != 0) {
            
            fn = fn->next;
        }
        
        if (fn == NULL) { // this is a new function
                
            fn = VG_(calloc)("fn", sizeof(Function), 1);
            
            fn->key = hash;
            fn->name = name;
            fn->input_map = HT_construct(NULL);
            fn->function_id = r->next_function_id++;
            
            HT_add_node(fn_ht, fn->key, fn);
            
        } else
            VG_(free)(name);
        
        if (fn->obj == NULL) { // new object
                
            UInt hash_obj = APROF_(str_hash)(obj_name);
            Object * obj = HT_lookup(obj_ht, hash_obj);
            while (obj != NULL && VG_(strcmp)(obj->name, obj_name) != 0) {
                
                obj = obj->next;
            }
            
            if (obj == NULL) {
                
                obj = VG_(calloc)("obj", sizeof(Object), 1);
                
                obj->key = hash_obj;
                obj->name = obj_name;
                obj->filename = NULL; /* FixMe */
                HT_add_node(obj_ht, obj->key, obj);
                
                fn->obj = obj;
                
            } else
                VG_(free)(obj_name);
            
            fn->obj = obj;
            
        } else
            VG_(free)(obj_name);

        curr = fn;
        
    } else if (token[0] == 'd') {
        
        APROF_(assert)(curr != NULL, "mangled name without a valid fn: %s", line_orig);
        
        //  routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, &error);
        APROF_(assert)(id > 0, "Invalid ID: %s", line_orig);
        UOF_LONG(error, token, id, line_orig);
        
        APROF_(assert)(*rid == id, "Routine id mismatch: %s", line_orig);
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid mangled name: %s", line_orig);
        
        // function mangled name
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        
        APROF_(assert)(VG_(strlen)(token) > 0, "Invalid mangled name: %s", line_orig);
        
        if (curr->mangled != NULL) {
            APROF_(assert)(VG_(strcmp)(curr->mangled, token) == 0,
                    "different mangled: %s <=> %s", curr->mangled, token);
        } else {
            curr->mangled = VG_(strdup)("mangled", token);
        }
        
    } else if (token[0] == 'p') {
        
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, &error);
        APROF_(assert)(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(error, token, id, line_orig);
        
        APROF_(assert)(*rid == id, "Routine id mismatch: %s", line_orig);
        
        // input size
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid input size: %s", line_orig);
        ULong input_size = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, input_size, line_orig);
        
        // min
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid min: %s", line_orig);
        ULong min = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, min, line_orig);
        WARNING(min > 0, "Invalid min: %s", line_orig);

        // max
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid max: %s", line_orig);
        ULong max = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, max, line_orig);
        WARNING(max > 0 && max >= min, "Invalid max: %s", line_orig);
        
        // sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sum: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, sum, line_orig);
        WARNING(sum >= max, "Invalid sum: %s", line_orig);
        
        // sqr sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sqr sum: %s", line_orig);
        double sqr_sum = VG_(strtod)(token, NULL);
        UOF_DOUBLE(error, token, sqr_sum, line_orig);
        WARNING(sqr_sum >= sum, "Invalid sqr_sum: %s", line_orig);
        
        // occ
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid occ: %s", line_orig);
        ULong occ = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, occ, line_orig);
        WARNING(occ > 0, "Invalid occ: %s", line_orig);
        
        WARNING(sum >= min*occ, "Invalid sum: %s", line_orig);
        
        HELPER(if (sqr_sum < ((double) min) * ((double) min) * ((double) occ)
                && !r->sqr_cumul_overflow) {
            
            WARNING(0, "Invalid sqr_sum (overflow?): %s\n", report);
            r->sqr_cumul_overflow = True;
        })
        
        // cumul_real
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid cumul: %s", line_orig);
        ULong cumul_real = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, cumul_real, line_orig);
        WARNING(cumul_real >= 0 && cumul_real <= sum, "Invalid cumul: %s", line_orig);

        // self_total
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self total: %s", line_orig);
        ULong self = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self, line_orig);
        WARNING(self > 0 && self <= sum, "Invalid self total: %s", line_orig);
        
        // self_min
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self min: %s", line_orig);
        ULong self_min = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self_min, line_orig);
        WARNING(self_min > 0 && self_min <= min, "Invalid self min: %s", line_orig);
        
        WARNING(self >= self_min*occ, "Invalid self total: %s", line_orig);
        
        // self_max
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self max: %s", line_orig);
        ULong self_max = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self_max, line_orig);
        WARNING(self_max >= self_min && self_max <= max, "Invalid self max: %s", line_orig);
        
        // sqr self
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sqr self: %s", line_orig);
        double self_sqr = VG_(strtod)(token, NULL);
        UOF_DOUBLE(error, token, self_sqr, line_orig);
        
        HELPER(if (self_sqr < ((double) self_min) * ((double) self_min)
                && !r->sqr_self_overflow) {
            
            WARNING(0, "Invalid self sqr (overflow?): %s\n", report);
            r->sqr_self_overflow = True;
        })
        
        ULong sum_cumul_syscall = 0;
        ULong sum_cumul_thread = 0;
        ULong sum_self_syscall = 0;
        ULong sum_self_thread = 0;
        do {
            
            // input_size sum (deprecated)
            token = VG_(strtok)(NULL, DELIM_DQ);
            if (token == NULL) break;
            
            // input_size sqr (deprecated)
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid input_size sum sqr: %s", line_orig);
            
            // sum cumulative syscall input
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid sum cumulative syscall input: %s", line_orig);
            sum_cumul_syscall = VG_(strtoull10) (token, NULL);
            UOF_LONG(error, token, sum_cumul_syscall, line_orig);
            WARNING(sum_cumul_syscall <= input_size*occ, "invalid sum cumulative syscall inpur: %s", line_orig);
            
            // sum cumulative thread input
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid sum cumulative thread input: %s", line_orig);
            sum_cumul_thread = VG_(strtoull10) (token, NULL);
            UOF_LONG(error, token, sum_cumul_thread, line_orig);
            WARNING(sum_cumul_thread <= input_size*occ, "invalid sum cumulative thread input: %s", line_orig);
            
            WARNING(sum_cumul_thread + sum_cumul_syscall <= input_size * occ,
                    "invalid rvms syscall/thread input: %s", line_orig);
            
            // sum self syscall input
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL,
                    "Invalid rvms syscall self: %s", line_orig);
            sum_self_syscall = VG_(strtoull10) (token, NULL);
            UOF_LONG(error, token, sum_self_syscall, line_orig);
            WARNING(sum_self_syscall <= input_size*occ, "sum self syscall input: %s", line_orig);
            
            // sum self thread input
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid rvms thread: %s", line_orig);
            sum_self_thread = VG_(strtoull10) (token, NULL);
            UOF_LONG(error, token, sum_self_thread, line_orig);
            WARNING(sum_self_thread <= input_size*occ, "invalid sum self thread input: %s", line_orig);
            
            WARNING(sum_self_thread + sum_self_syscall <= input_size * occ,
                    "invalid self syscall/thread input: %s", line_orig);
            
        } while (0);
        
        APROF_(assert)(curr != NULL, "Invalid routine: %s", line_orig);
        
        UWord key = input_size;
        Input * tuple = HT_lookup(curr->input_map, key);
        if (tuple == NULL) {
            
            tuple = APROF_(new)(INPUT_S, sizeof(Input));
            
            tuple->min_cumulative_cost = (ULong)-1;
            tuple->min_self_cost = (ULong)-1;
            tuple->key = key;
            tuple->input_size = input_size;
            tuple->calls = 0;
            APROF_(assert)(tuple->calls == 0, "AHAH");
            
            HT_add_node(curr->input_map, key, tuple);
        }
                
        ADD(tuple->sum_cumulative_cost, sum);
        ADD_D(tuple->sqr_cumulative_cost, sqr_sum);
        ADD(tuple->calls, occ);
        HELPER(ADD(curr->total_calls, tuple->calls));
        
        WARNING(tuple->sqr_cumulative_cost >= tuple->sum_cumulative_cost,
                "Invalid sqr_sum: %s", line_orig);
        
        if (tuple->max_cumulative_cost < max)
            tuple->max_cumulative_cost = max;
        
        if (tuple->min_cumulative_cost > min)
            tuple->min_cumulative_cost = min;
        
        WARNING(tuple->max_cumulative_cost >= tuple->min_cumulative_cost,
                "Invalid min/max");
        WARNING(tuple->sum_cumulative_cost >= tuple->max_cumulative_cost,
                "Invalid sum");
        WARNING(tuple->sum_cumulative_cost >= tuple->min_cumulative_cost * tuple->calls,
                "Invalid sum");
                
        HELPER(if (tuple->sqr_cumulative_cost 
                    <= ((double) tuple->min_cumulative_cost)
                        * ((double) tuple->min_cumulative_cost)
                        * ((double) tuple->calls)
                && !r->sqr_cumul_overflow) {
            
            WARNING(0, " Invalid sqr_sum (overflow?): %s\n", report);
            r->sqr_cumul_overflow = True;
        })
        
        ADD(tuple->sum_cumul_real_cost, cumul_real);
        HELPER(ADD(r->total_real_cost, cumul_real));
        
        ADD(tuple->sum_self_cost, self);
        HELPER(ADD(r->total_self_cost, self));
        
        ADD_D(tuple->sqr_self_cost, self_sqr);
        
        APROF_(assert)(tuple->sum_cumul_real_cost <= tuple->sum_cumulative_cost,
                "Invalid real sum cost: %s", line_orig);
        
        if (tuple->min_self_cost > self_min)
            tuple->min_self_cost = self_min;
        
        if (tuple->max_self_cost < self_max)
            tuple->max_self_cost = self_max;
        
        APROF_(assert)(
                tuple->min_self_cost <= tuple->min_cumulative_cost 
                && tuple->max_self_cost <= tuple->max_cumulative_cost 
                && tuple->sum_self_cost <= tuple->sum_cumulative_cost,
                "Invalid self: %s", line_orig);
        
        APROF_(assert)(tuple->max_self_cost >= tuple->min_self_cost,
                "Invalid self min/max");
        APROF_(assert)(tuple->sum_self_cost >= tuple->max_self_cost,
                "Invalid self sum");
        APROF_(assert)(tuple->sum_self_cost >= tuple->min_self_cost * tuple->calls,
                "Invalid self sum: %llu %llu %llu", tuple->sum_self_cost, tuple->min_self_cost, tuple->calls);
        
        HELPER(if (tuple->sqr_self_cost 
                    <= ((double) tuple->min_self_cost)
                        * ((double) tuple->min_self_cost)
                        * ((double) tuple->calls)
                && !r->sqr_self_overflow) {
            
            WARNING(0, "Invalid sqr_sum (overflow?): %s\n", report);
            r->sqr_self_overflow = True;
        })
        
        if (input_metric == DRMS) {
            
            ADD(tuple->sum_cumul_thread, sum_cumul_thread);
            ADD(tuple->sum_cumul_syscall, sum_cumul_syscall);
            
            APROF_(assert)(
                    tuple->sum_cumul_thread + tuple->sum_cumul_syscall <= input_size * tuple->calls,
                    "invalid rvms syscall/thread: %s", line_orig);
            
            ADD(tuple->sum_self_thread, sum_self_thread);
            ADD(tuple->sum_self_syscall, sum_self_syscall);
            
            APROF_(assert)(
                    tuple->sum_self_thread + tuple->sum_self_syscall <= input_size * tuple->calls,
                    "invalid rvms syscall/thread: %s", line_orig);
            
        }
        
    } else if (token[0] == 'a') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL && VG_(strlen)(token) > 0,
                "invalid app name: %s", line_orig);
        HChar app_r[1024] = { 0 };
        while (token != NULL) {
            
            APROF_(assert)(VG_(strlen)(app_r) + VG_(strlen)(token) + 1 < 1024,
                    "app name too long: %s", line_orig);
            
            VG_(strcat)(app_r, token);
            VG_(strcat)(app_r, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
        }
        
        // remove final space
        app_r[VG_(strlen)(app_r) - 1] = '\0';
        
        if (app == NULL) {
            
            app = VG_(strdup)("app_r", app_r);
            
        } else {
            
            if (VG_(strcmp)(app, app_r) != 0 HELPER(&& !r->merge_all)) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
        }
        
    } else if (token[0] == 'k') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid perf metric: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, sum, line_orig);
        APROF_(assert)(sum > 0, "Invalid sum: %s", line_orig);
        
        // wait to consider sum... maybe we discard this report...
        *total_cost = sum;
        
    } else if (token[0] == 'v') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid version: %s", line_orig);
        ULong ver = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, ver, line_orig);
        
        if (ver != REPORT_VERSION) {
            *invalid = True;
            VG_(free)(line);
            return NULL;
        }
            
    } else if (token[0] == 'q' || token[0] == 'x') {
        
        // Merge of reports with CCT is not yet supported
        *invalid = True;
        VG_(free)(line);
        return NULL;
        
    } else if (token[0] == 'i') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL,"Invalid input metric: %s", line_orig);

        input_metric_t im = INVALID;
        if (VG_(strcmp)(token, "rms") == 0) im = RMS;
        else if (VG_(strcmp)(token, "drms") == 0) im = DRMS;

        if (input_metric == RMS || input_metric == DRMS) {
            
            if (im != input_metric) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
            
        } else
            input_metric = im;
        
    } else if (token[0] == 't') {

        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid memory resolution: %s", line_orig);
        UInt m_res = VG_(strtoull10) (token, NULL);
        APROF_(assert)(
                m_res == 1 || m_res == 2 || m_res == 4 || m_res == 8 || m_res == 16,
                "Invalid memory resolution: %s", line_orig);
        
        if (memory_resolution != m_res) {
            *invalid = True;
            VG_(free)(line);
            return NULL;
        }

    } else if (token[0] == 'f') {
        
        // we allow different command lines
        
        /*
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL && VG_(strlen)(token) > 0,
                "invalid cmd name: %s", line_orig);
        HChar cmd_r[1024] = { 0 };
        while (token != NULL) {
            
            APROF_(assert)(VG_(strlen)(cmd_r) + VG_(strlen)(token) + 1 < 1024,
                    "cmd name too long: %s", line_orig);
            
            VG_(strcat)(cmd_r, token);
            VG_(strcat)(cmd_r, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
        }
        
        // remove final space
        cmd_r[VG_(strlen)(cmd_r) - 1] = '\0';
        
        if (cmd == NULL) {
            
            cmd = VG_(strdup)("cmd_r", cmd_r);
            
        } else {
            
            if (VG_(strcmp)(cmd, cmd_r) != 0 HELPER(&& !merge_all)) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
        }
        */

    } else if (token[0] == 'e') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid binary time: %s", line_orig);
        ULong bmtime = VG_(strtoull10) (token, NULL);
        
        if (bmtime != binary_mtime) {
            *invalid = True;
            VG_(free)(line);
            return NULL;
        }
        
    } else if (token[0] == 'c' || token[0] == 'm' || token[0] == 'u') {
        
        // ignored...
        
    } else {
        APROF_(assert)(0, "Unknown tag %c %s", token[0], line_orig);
    }
    
    VG_(free)(line);
    return curr;
}

Bool APROF_(merge_report)(HChar * report, Runtime * runtime) {

    //VG_(umsg)("Try to merge report: %s\n", report);
    HChar buf[4096];
    
    // check memory resolution
    if (APROF_(get_memory_resolution_report)(report) != runtime->memory_resolution) 
        return False;
    
    SysRes res = VG_(open)(report, VKI_O_RDONLY, VKI_S_IRUSR | VKI_S_IWUSR);
    if (sr_isError(res)) return False;
    Int file = (Int) sr_Res(res);
    
    HChar line[1024];
    UInt offset = 0;
    Function * current_routine = NULL;
    UInt curr_rid = 0;
    Bool invalid = False;
    ULong total_cost = 0;
    
    HELPER(runtime->sqr_cumul_overflow = False);
    HELPER(runtime->sqr_self_overflow = False);
    
    /* merge tuples */
    while (1) {
        
        Int r = VG_(read)(file, buf, 4096);
        if (r < 0) {
            
            VG_(umsg)("Error when reading %s\n", report);
            return False;
            
        } else if (r == 0)
            break; /* EOF */
        
        Int i;
        for (i = 0; i < r; i++) {
            
            if (buf[i] == '\n') {
                
                line[offset++] = '\0';
                current_routine = APROF_(merge_tuple)(  line, 
                                                        current_routine, 
                                                        report,
                                                        runtime,
                                                        &curr_rid, 
                                                        &invalid, 
                                                        &total_cost);
                
                offset = 0;
                
                if (invalid) {
                    VG_(close)(file);
                    return False;
                }
                
            } else
                line[offset++] = buf[i];
            
            APROF_(assert)(offset < 1024, "Line too long");
        }
    }
    
    line[offset++] = '\0';
    
    current_routine = APROF_(merge_tuple)(  line, 
                                            current_routine, 
                                            report,
                                            runtime,
                                            &curr_rid, 
                                            &invalid, 
                                            &total_cost);
    
    if (invalid)
        return False;
    
    HELPER(post_merge_consistency(runtime, report));
    
    // delete merged report
    VG_(unlink)(report);
    
    return True;
}

#if APROF_TOOL
void APROF_(load_reports)(void) {

    UInt k;
    HChar ** list;
    
    UInt size = APROF_(search_reports)(&list);
    for (k = 0; k < size; k++) {
        Bool m = APROF_(merge_report)(list[k], &APROF_(runtime));
        if (m) VG_(umsg)("Merged report: %s\n", list[k]);
    }
    VG_(free)(list);
}
#endif // APROF_TOOL
