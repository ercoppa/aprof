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

#include "aprof.h"
#include "aprof_inline.h"

#define DELIM_SQ '$'
#define DELIM_DQ "$"

#define UOF_LONG(error, token, val, line) APROF_(assert)(!(val == 0 && error == token), \
                                                            "under/over flow: %s", line);

#if !APROF_TOOL
    #define HELPER(action) action
#else
    #define HELPER(action)
#endif

static UInt APROF_(search_reports)(HChar *** reports) {

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

__attribute__((__unused__))
static Int APROF_(get_pid_report)(HChar * report) {
    
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
    
    // skip memory resolution and "_"
    while (p > rep && *p != '_') p--;
    if (!(p-- > rep)) return -1;
    
    // skip TID and "_"
    while (p > rep && *p != '_') p--;
    if (!(p-- > rep)) return -1;
    
    // get PID
    UInt pos = p - rep + 1;
    if (!(pos > 0)) return -1;
    while (p >= rep && *p != '_') p--;
    if (!(p < rep + pos)) return -1;
    
    p++;
    Int pid = VG_(strtoull10)(p, NULL);
    
    return pid;
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
                                        UInt * rid,
                                        #if !APROF_TOOL
                                        aprof_report * r,
                                        #endif
                                        Bool * invalid,
                                        ULong * real_cumul) {
    
    #if APROF_TOOL
    HashTable * fn_ht = APROF_(runtime).fn_ht;
    HashTable * obj_ht = APROF_(runtime).obj_ht;
    HChar * cmd = APROF_(runtime).cmd_line;
    #else
    HashTable * fn_ht = r->fn_ht;
    HashTable * fn_ht = r->obj_ht;
    HChar * cmd = r->cmd;
    #endif
    
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
        
        if (curr != NULL) {
            if (*real_cumul == 0)
                HELPER(EMSG(YELLOW("Warning:") "Invalid cumul: %s", line_orig));
        } else 
            HELPER(ADD(r->performance_metric, *real_cumul));
        
        *real_cumul = 0;
        
        // function name
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid fn name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        APROF_(debug_assert)(VG_(strlen)(token) > 0, "Invalid fn name: %s", line_orig);
        HChar * name = VG_(strdup)("fn_name", token);
        
        // object name
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid obj name: %s", line_orig);
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        APROF_(debug_assert)(VG_(strlen)(token) > 0, "Invalid obj name: %s", line_orig);
        HChar * obj_name = VG_(strdup)("obj_name", token);
        
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, &error);
        APROF_(debug_assert)(id >= 0, "Invalid ID: %s", line_orig);
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
            APROF_(debug_assert)(fn != NULL, "Function not allocable");
            
            fn->key = hash;
            fn->name = name;
            
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
                APROF_(debug_assert)(obj != NULL, "Obj not allocable");
                
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
        APROF_(debug_assert)(id > 0, "Invalid ID: %s", line_orig);
        UOF_LONG(error, token, id, line_orig);
        
        APROF_(assert)(*rid == id, "Routine id mismatch: %s", line_orig);
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid mangled name: %s", line_orig);
        
        // function mangled name
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        
        APROF_(debug_assert)(VG_(strlen)(token) > 0, "Invalid mangled name: %s", line_orig);
        
        if (curr->mangled != NULL) {
            APROF_(assert)(VG_(strcmp)(curr->mangled, token) == 0,
                    "different mangled: %s <=> %s", curr->mangled, token);
        } else {
            curr->mangled = VG_(strdup)("mangled", token);
        }
        
    } else if (token[0] == 'p') {
        #if 0
        // routine ID
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid id: %s", line_orig);
        ULong id = VG_(strtoull10)(token, &error);
        APROF_(assert)(id >= 0, "Invalid ID: %s", line_orig);
        UOF_LONG(error, token, id, line_orig);
        
        APROF_(assert)(*rid == id, "Routine id mismatch: %s", line_orig);
        
        UInt i;
        for (i = 0; i < sizeof(rtn_skip) / sizeof(HChar *); i++) {
            if (VG_(strcmp)(rtn_skip[i], curr->fn->name) == 0) {
                VG_(free)(line);
                r->tmp = 1;
                return curr;
            }
        }
        
        // RMS
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid rms: %s", line_orig);
        ULong rms = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, rms, line_orig);
        
        // min
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid min: %s", line_orig);
        ULong min = VG_(strtoull10) (token, NULL);
        APROF_(assert)(min != LLONG_MIN && min != LLONG_MAX,
                "under/overflow: %s", line_orig);
        APROF_(assert)(min > 0, "Invalid min: %s", line_orig);
        
        // max
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid max: %s", line_orig);
        ULong max = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, max, line_orig);
        APROF_(assert)(max > 0 && max >= min, "Invalid max: %s", line_orig);
        
        // sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sum: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, sum, line_orig);
        APROF_(assert)(sum >= max, "Invalid sum: %s", line_orig);
        
        // sqr sum
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sqr sum: %s", line_orig);
        double sqr_sum = VG_(strtod)(token, NULL);
        UOF_DOUBLE(sqr_sum, line_orig);
        APROF_(assert)(sqr_sum >= sum, "Invalid sqr_sum: %s", line_orig);
        
        // occ
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid occ: %s", line_orig);
        ULong occ = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, occ, line_orig);
        APROF_(assert)(occ > 0, "Invalid occ: %s", line_orig);
        
        APROF_(assert)(sum >= min*occ, "Invalid sum: %s", line_orig);
        
        if (sqr_sum < ((double) min) * ((double) min) * ((double) occ)
                && !r->sqr_over) {
            
            EMSG(
                    YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", report);
            
            r->sqr_over = True;
        }
        
        // cumul_real
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid cumul: %s", line_orig);
        ULong cumul_real = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, cumul_real, line_orig);
        APROF_(assert)(cumul_real >= 0 && cumul_real <= sum,
                "Invalid cumul: %s", line_orig);
        
        ADD(r->tmp, cumul_real);
        
        // self_total
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self total: %s", line_orig);
        ULong self = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self, line_orig);
        APROF_(assert)(self > 0 && self <= sum, "Invalid self total: %s", line_orig);
        
        // self_min
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self min: %s", line_orig);
        ULong self_min = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self_min, line_orig);
        APROF_(assert)(self_min > 0 && self_min <= min,
                "Invalid self min: %s", line_orig);
        
        APROF_(assert)(self >= self_min*occ, "Invalid self total: %s", line_orig);
        
        // self_max
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid self max: %s", line_orig);
        ULong self_max = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, self_max, line_orig);
        APROF_(assert)(self_max >= self_min && self_max <= max,
                "Invalid self max: %s", line_orig);
        
        // sqr self
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid sqr self: %s", line_orig);
        double self_sqr = VG_(strtod)(token, NULL);
        UOF_DOUBLE(self_sqr, line_orig);
        
        if (self_sqr < ((double) self_min) * ((double) self_min)
                && !r->self_sqr_over) {
            
            EMSG(
                    YELLOW("Warning:") " Invalid self sqr (overflow?): %s\n", report);
            
            r->self_sqr_over = True;
        }
        
        ULong rms_sum = 0;
        double rms_sqr = 0;
        ULong rvms_syscall_sum = 0;
        ULong rvms_thread_sum = 0;
        ULong rvms_syscall_self = 0;
        ULong rvms_thread_self = 0;
        if (r->version == REPORT_VERSION && r->input_metric == RVMS) {
            
            // rms sum
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid rms sum: %s", line_orig);
            rms_sum = VG_(strtoull10) (token, NULL);
            UOF_LONG(error, token, rms_sum, line_orig);
            APROF_(assert)(rms_sum <= rms*occ, "invalid rms sum: %s", line_orig);
            
            // ratio sum sqr
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid sqr rms: %s", line_orig);
            rms_sqr = VG_(strtod)(token, NULL);
            UOF_DOUBLE(rms_sqr, line_orig);
            APROF_(assert)(rms_sqr <= ((double)rms) * ((double)rms) * ((double)occ),
                    "invalid rms sqr: %s", line_orig);
            
            // rvms syscall sum
            token = VG_(strtok)(NULL, DELIM_DQ);
            if (token != NULL) {
                
                rvms_syscall_sum = VG_(strtoull10) (token, NULL);
                UOF_LONG(error, token, rvms_syscall_sum, line_orig);
                APROF_(assert)(rvms_syscall_sum <= rms*occ,
                        "invalid rvms syscall: %s", line_orig);
                
                // rvms thread sum
                token = VG_(strtok)(NULL, DELIM_DQ);
                APROF_(assert)(token != NULL, "Invalid rvms thread: %s", line_orig);
                rvms_thread_sum = VG_(strtoull10) (token, NULL);
                UOF_LONG(error, token, rvms_thread_sum, line_orig);
                APROF_(assert)(rvms_thread_sum <= rms*occ,
                        "invalid rvms thread: %s", line_orig);
                
                APROF_(assert)(rvms_thread_sum + rvms_syscall_sum <= rms * occ,
                        "invalid rvms syscall/thread: %s", line_orig);
                
                // rvms syscall self
                token = VG_(strtok)(NULL, DELIM_DQ);
                APROF_(assert)(token != NULL,
                        "Invalid rvms syscall self: %s", line_orig);
                rvms_syscall_self = VG_(strtoull10) (token, NULL);
                UOF_LONG(error, token, rvms_syscall_self, line_orig);
                APROF_(assert)(rvms_syscall_self <= rms*occ,
                        "invalid rvms syscall self: %s", line_orig);
                
                // rvms thread self
                token = VG_(strtok)(NULL, DELIM_DQ);
                APROF_(assert)(token != NULL, "Invalid rvms thread: %s", line_orig);
                rvms_thread_self = VG_(strtoull10) (token, NULL);
                UOF_LONG(error, token, rvms_thread_self, line_orig);
                APROF_(assert)(rvms_thread_self <= rms*occ,
                        "invalid rvms thread self: %s", line_orig);
                
                APROF_(assert)(rvms_thread_self + rvms_syscall_self <= rms * occ,
                        "invalid rvms syscall/thread self: %s", line_orig);
                
            }
            
        }
        
        APROF_(assert)(curr != NULL, "Invalid routine: %s", line_orig);
        RMSInfo * info_access = HT_lookup(curr->rvms_map, rms);
        if (info_access == NULL) {
            
            info_access =
                    (RMSInfo *) VG_(calloc)("rms_info", 1, sizeof(RMSInfo));
            APROF_(debug_assert)(info_access != NULL,
                    "rms_info not allocable in function exit");
            
            info_access->min_cumulative_time = min;
            info_access->self_time_min = self_min;
            info_access->key = rms;
            HT_add_node(curr->rvms_map, info_access->key, info_access);
            
        }
        
        ADD(info_access->cumulative_time_sum, sum);
        ADD_D(info_access->cumulative_sum_sqr, sqr_sum);
        ADD(info_access->calls_number, occ);
        ADD(curr->total_calls, info_access->calls_number);
        
        APROF_(assert)(
                info_access->cumulative_sum_sqr >= info_access->cumulative_time_sum,
                "Invalid sqr_sum: %s", line_orig);
        
        if (info_access->max_cumulative_time < max)
            info_access->max_cumulative_time = max;
        
        if (info_access->min_cumulative_time > min)
            info_access->min_cumulative_time = min;
        
        APROF_(assert)(
                info_access->max_cumulative_time >= info_access->min_cumulative_time,
                "Invalid min/max");
        APROF_(assert)(
                info_access->cumulative_time_sum >= info_access->max_cumulative_time,
                "Invalid sum");
        
        APROF_(assert)(
                info_access->cumulative_time_sum >= info_access->min_cumulative_time * info_access->calls_number,
                "Invalid sum");
        
        if (info_access->cumulative_sum_sqr
                < ((double) info_access->min_cumulative_time)
                        * ((double) info_access->min_cumulative_time)
                        * ((double) info_access->calls_number)
                && !r->sqr_over) {
            
            EMSG(
                    YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", report);
            
            r->sqr_over = True;
        }
        
        ADD(info_access->cumul_real_time_sum, cumul_real);
        ADD(r->real_total_cost, cumul_real);
        
        ADD(info_access->self_time_sum, self);
        ADD(r->self_total_cost, self);
        ADD_D(info_access->self_sum_sqr, self_sqr);
        
        APROF_(assert)(
                info_access->cumul_real_time_sum >= 0 && info_access->cumul_real_time_sum <= info_access->cumulative_time_sum,
                "Invalid cumul: %s", line_orig);
        
        if (info_access->self_time_min > self_min)
            info_access->self_time_min = self_min;
        
        if (info_access->self_time_max < self_max)
            info_access->self_time_max = self_max;
        
        APROF_(assert)(
                info_access->self_time_min <= info_access->min_cumulative_time && info_access->self_time_max <= info_access->max_cumulative_time && info_access->self_time_sum <= info_access->cumulative_time_sum,
                "Invalid self: %s", line_orig);
        
        APROF_(assert)(info_access->self_time_max >= info_access->self_time_min,
                "Invalid self min/max");
        APROF_(assert)(info_access->self_time_sum >= info_access->self_time_max,
                "Invalid self sum");
        
        APROF_(assert)(
                info_access->self_time_sum >= info_access->self_time_min * info_access->calls_number,
                "Invalid sum");
        
        if (info_access->self_sum_sqr
                < ((double) info_access->self_time_min)
                        * ((double) info_access->self_time_min)
                        * ((double) info_access->calls_number)
                && !r->self_sqr_over) {
            
            EMSG(
                    YELLOW("Warning:") " Invalid sqr_sum (overflow?): %s\n", report);
        }
        
        if (r->version == REPORT_VERSION && r->input_metric == RVMS) {
            
            ADD(info_access->rms_input_sum, rms_sum);
            ADD_D(info_access->rms_input_sum_sqr, rms_sqr);
            
            APROF_(assert)(
                    info_access->rms_input_sum <= rms * info_access->calls_number,
                    "invalid rms sum");
            
            APROF_(assert)(
                    info_access->rms_input_sum_sqr <= ((double)rms) * ((double)rms) * ((double)info_access->calls_number),
                    "invalid rms sqr");
            
            ADD(info_access->rvms_thread_sum, rvms_thread_sum);
            ADD(info_access->rvms_syscall_sum, rvms_syscall_sum);
            
            APROF_(assert)(
                    info_access->rvms_thread_sum + info_access->rvms_syscall_sum <= rms * info_access->calls_number,
                    "invalid rvms syscall/thread: %s", line_orig);
            
            ADD(info_access->rvms_thread_self, rvms_thread_self);
            ADD(info_access->rvms_syscall_self, rvms_syscall_self);
            
            APROF_(assert)(
                    info_access->rvms_thread_self + info_access->rvms_syscall_self <= rms * info_access->calls_number,
                    "invalid rvms syscall/thread: %s", line_orig);
            
        }
        #endif
    } else if (token[0] == 'a') {
        
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL && VG_(strlen)(token) > 0,
                "invalid cmd name: %s", line_orig);
        HChar app[1024] = { 0 };
        while (token != NULL) {
            
            APROF_(assert)(VG_(strlen)(app) + VG_(strlen)(token) + 1 < 1024,
                    "cmd name too long: %s", line_orig);
            
            VG_(strcat)(app, token);
            VG_(strcat)(app, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
        }
        
        // remove final space
        app[VG_(strlen)(app) - 1] = '\0';
        
        if (cmd == NULL) {
            
            cmd = VG_(strdup)("app", app);
            
        } else {
            
            if (VG_(strcmp)(cmd, app) != 0 HELPER(&& !merge_all)) {
                *invalid = True;
                VG_(free)(line);
                return NULL;
            }
            
            /*
             APROF_(assert)(VG_(strcmp)(app, r->app) == 0, 
             "different app");
             */
        }
        
    } else if (token[0] == 'k') {
        #if 0
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid perf metric: %s", line_orig);
        ULong sum = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, sum, line_orig);
        APROF_(assert)(sum > 0, "Invalid sum: %s", line_orig);
        
        /*
         * merge_tuple is used also to test if a report needs
         * to be merged with the current loaded report. We need to
         * check report tag f so we sum (maybe) the performance metric
         * when we see the first routine
         */
        r->tmp = 0;
        ADD(r->tmp, sum);
        #endif
    } else if (token[0] == 'v') {
        #if 0
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid version: %s", line_orig);
        ULong ver = VG_(strtoull10) (token, NULL);
        UOF_LONG(error, token, ver, line_orig);
        APROF_(assert)(ver == REPORT_VERSION || ver == REPORT_VERSION_OLD,
                "Invalid version: %s", line_orig);
        
        if (r->version == 0)
            r->version = ver;
        if (merge_runs || merge_threads || compare) {
            
            if (compare && r == &ap_rep[1]) {
                
                /*
                 APROF_(assert)(ap_rep[0].version == r->version, 
                 "You are elaborating reports of different versions: %s", 
                 line_orig);
                 */

            } else {
                
                if (r->version != ver) {
                    if (merge_all)
                        EMSG("reports with different versions");
                    *invalid = True;
                    VG_(free)(line);
                    return NULL;
                }
                
                /*
                 APROF_(assert)(r->version == ver, 
                 "You are elaborating reports of different versions: %s", 
                 line_orig);
                 */
            }
            
        }
        
        if (r->version == REPORT_VERSION_OLD)
            r->input_metric = RMS;
        #endif
    } else if (token[0] == 'q' || token[0] == 'x') {
        
        APROF_(assert)(0, "Merge of reports with CCT is not yet supported");
        
    } else if (token[0] == 'i') {
        #if 0
        if (r->version == REPORT_VERSION) {
            
            token = VG_(strtok)(NULL, DELIM_DQ);
            APROF_(assert)(token != NULL, "Invalid input metric: %s", line_orig);
            
            if (VG_(strcmp)("rms", token) == 0) {
                
                if (r->input_metric > 0)
                    APROF_(assert)(r->input_metric == RMS, "Invalid metric")
                
                r->input_metric = RMS;
                
            } else if (VG_(strcmp)("rvms", token) == 0) {
                
                if (r->input_metric > 0)
                    APROF_(assert)(r->input_metric == RVMS, "Invalid metric")
                
                r->input_metric = RVMS;
                
            } else
                APROF_(assert)(0, "Invalid input metric: %s", line_orig);
            
        } else if (r->version == REPORT_VERSION_OLD)
            r->input_metric = RMS;
        else
            APROF_(assert)(0, "Invalid metric: %s", line_orig);
        #endif
    } else if (token[0] == 'c' || token[0] == 'e' || token[0] == 'm'
            || token[0] == 'u') {
        
        // ignored...
        
    } else if (token[0] == 't') {
        #if 0
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL, "Invalid memory resolution: %s", line_orig);
        UInt m_res = VG_(strtoull10) (token, NULL);
        APROF_(assert)(
                m_res == 1 || m_res == 2 || m_res == 4 || m_res == 8 || m_res == 16,
                "Invalid memory resolution: %s", line_orig);
        
        if (r->memory_resolution != m_res) {
            if (merge_all)
                EMSG("reports with different memory resolutions");
            *invalid = True;
            VG_(free)(line);
            return NULL;
        }
        #endif
    } else if (token[0] == 'f') {
        #if 0
        token = VG_(strtok)(NULL, DELIM_DQ);
        APROF_(assert)(token != NULL && VG_(strlen)(token) > 0,
                "invalid cmd name: %s", line_orig);
        HChar app[1024] = { 0 };
        while (token != NULL) {
            
            APROF_(assert)(VG_(strlen)(app) + VG_(strlen)(token) + 1 < 1024,
                    "cmd name too long: %s", line_orig);
            
            VG_(strcat)(app, token);
            VG_(strcat)(app, " ");
            token = VG_(strtok)(NULL, DELIM_DQ);
            
        }
        
        // remove final space
        app[VG_(strlen)(app) - 1] = '\0';
        
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
             APROF_(assert)(VG_(strcmp)(app, r->cmd) == 0, 
             "different command");
             */
        }
        #endif
    } else {
        
        APROF_(assert)(0, "Unknown tag %c %s", token[0], line_orig);
        
    }
    
    VG_(free)(line);
    return curr;
}

#if APROF_TOOL
static Bool APROF_(merge_report)(HChar * report) {
#else
static Bool APROF_(merge_report)(HChar * report, aprof_report * rep_data) {
#endif    

    VG_(umsg)("Try to merge: %s\n", report);
    HChar buf[4096];
    
    // check memory resolution
    if (APROF_(get_memory_resolution_report)(report) !=
            APROF_(runtime).memory_resolution) 
        return False;
    
    SysRes res = VG_(open)(report, VKI_O_RDONLY, VKI_S_IRUSR | VKI_S_IWUSR);
    if (sr_isError(res)) return False;
    Int file = (Int) sr_Res(res);
    
    HChar line[1024];
    UInt offset = 0;
    Function * current_routine = NULL;
    UInt curr_rid = 0;
    Bool invalid = False;
    ULong real_cumul;
    
    HELPER(rep_data->sqr_over = False);
    HELPER(rep_data->self_sqr_over = False);
    
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
                #if APROF_TOOL
                current_routine = APROF_(merge_tuple)(  line, 
                                                        current_routine, 
                                                        report, 
                                                        &curr_rid, 
                                                        &invalid, 
                                                        &real_cumul);
                #else
                current_routine = APROF_(merge_tuple)(  line, 
                                                        current_routine, 
                                                        report, 
                                                        rep_data, 
                                                        &curr_rid, 
                                                        &real_cumul,
                                                        &invalid);
                #endif
                
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
    
    #if APROF_TOOL
    current_routine = APROF_(merge_tuple)(  line, 
                                            current_routine, 
                                            report, 
                                            &curr_rid, 
                                            &invalid, 
                                            &real_cumul);
    #else
    current_routine = APROF_(merge_tuple)(  line, 
                                            current_routine, 
                                            report, 
                                            rep_data, 
                                            &curr_rid, 
                                            &real_cumul,
                                            &invalid);
    #endif
    
    VG_(close)(file);
    
    if (invalid)
        return False;
    
    HELPER(post_merge_consistency(rep_data, report));
    
    return True;
}

void APROF_(load_reports)(void) {

    UInt k;
    HChar ** list;
    
    UInt size = APROF_(search_reports)(&list);
    for (k = 0; k < size; k++)
        APROF_(merge_report)(list[k]);
        
    VG_(free)(list);
}
