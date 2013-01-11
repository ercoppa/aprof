/*
 * Report generator
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */
 
 /*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2013, Emilio Coppa (ercoppa@gmail.com),
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

// last modification time of the current program
static ULong APROF_(binary_time) = 0;

static HChar * put_delim(HChar * str, Int size) {
    
    Int skip = 0;
    Int i = 0;
    for (i = 0; i < size; i++) {
        
        if (str[i] == ' ' && !skip)
            str[i] = '@';
        
        else if (str[i] == '"')
            skip = ~skip;
        
        else if (str[i] == '\0')
            return str;
    }
    
    return str;
}

#if CCT == 0
static Function * merge_tuple(HChar * line, Int size, 
                            Function * curr, ThreadData * tdata) {
    
    if (size <= 0) return curr;
    line = put_delim(line, size);
    
    HChar * token = VG_(strtok)(line, "@");
    if (token == NULL) return curr;
    
    /* FixMe check exec mtime */
    
    if (token[0] == 'r') {
        
        // function name
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        HChar * name = VG_(strdup)("fn_name", token);
        
        // object name
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        token[VG_(strlen)(token) - 1] = '\0'; // remove last "
        token++; // skip first "
        HChar * obj_name = VG_(strdup)("obj_name", token);
        
        // Search function
        UInt hash = APROF_(str_hash)((Char *)name);
        curr = HT_lookup(APROF_(fn_ht), hash);
        while (curr != NULL 
                && VG_(strcmp)((HChar *)curr->name, (HChar *)name) != 0) {
                    
            curr = curr->next;
        }
        
        //VG_(printf)("Parsed: %s %s\n", name, obj_name);
        
        if (curr == NULL) { // this is a new function
            
            //VG_(printf)("New Function: %s\n", name);
            
            curr = VG_(calloc)("fn", sizeof(Function), 1);
            #if DEBUG
            AP_ASSERT(curr != NULL, "Function not allocable");
            #endif
            
            curr->key = hash;
            curr->name = name;
            
            HT_add_node(APROF_(fn_ht), curr->key, curr);
            
        }
        
        if (curr->obj == NULL) { // new object
            
            UInt hash_obj = APROF_(str_hash)((Char *)obj_name);
            Object * obj = HT_lookup(APROF_(obj_ht), hash_obj);
            while (obj != NULL 
                && VG_(strcmp)((HChar *)obj->name, (HChar *)obj_name) != 0) {
                
                obj = obj->next;
            }
            
            //VG_(printf)("New object: %s\n", obj_name);
            
            obj = VG_(calloc)("obj", sizeof(Object), 1);
            #if DEBUG
            AP_ASSERT(obj != NULL, "Obj not allocable");
            #endif
            
            obj->key = hash_obj;
            obj->name = obj_name;
            obj->filename = NULL; /* FixMe */
            HT_add_node(APROF_(obj_ht), obj->key, obj);
            
            curr->obj = obj;
            
        }
        
        RoutineInfo * rtn_info = HT_lookup(tdata->routine_hash_table, (UWord)curr);
        if (rtn_info == NULL) {
            
            rtn_info = APROF_(new_routine_info)(tdata, curr, (UWord) curr);
        
        } 

    } else if (token[0] == 'd') { 
        
        if (curr == NULL || curr->mangled != NULL) 
            return curr;
        
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        
        // skip routine ID
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        
        // function mangled name
        token[VG_(strlen)((HChar *)token) - 1] = '\0'; // remove last "
        token++; // skip first "
        curr->mangled = VG_(strdup)("mangled", (HChar *)token);
    
    } else if (token[0] == 'p') {
        
        // routine ID
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        
        // RMS
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong rms = VG_(strtoull10) ((HChar *)token, NULL);
        if (rms == 0) return curr;
        
        // min
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong min = VG_(strtoull10) ((HChar *)token, NULL);
        if (min == 0) return curr;
        
        // max
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong max = VG_(strtoull10) ((HChar *)token, NULL);
        if (max == 0) return curr;
        
        // sum
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong sum = VG_(strtoull10) ((HChar *)token, NULL);
        if (sum == 0) return curr;
        
        // sqr sum
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong sqr_sum = VG_(strtoull10) ((HChar *)token, NULL);
        if (sqr_sum == 0) return curr;
        
        // occ
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong occ = VG_(strtoull10) ((HChar *)token, NULL);
        if (occ == 0) return curr;
        
        // cumul_real
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong cumul_real = VG_(strtoull10) ((HChar *)token, NULL);
        if (cumul_real == 0) return curr;
        
        // self_total
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong self = VG_(strtoull10) ((HChar *)token, NULL);
        if (self == 0) return curr;
        
        // self_min
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong self_min = VG_(strtoull10) ((HChar *)token, NULL);
        if (self_min == 0) return curr;
        
        // self_max
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong self_max = VG_(strtoull10) ((HChar *)token, NULL);
        if (self_max == 0) return curr;
        
        // sqr self
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong self_sqr = VG_(strtoull10) ((HChar *)token, NULL);
        if (self_sqr == 0) return curr;
        
        #if INPUT_METRIC == RVMS
        // ratio_sum
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong ratio = VG_(strtoull10) ((HChar *)token, NULL);
        if (ratio == 0) return curr;
        
        // ratio sum sqr
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong ratio_sqr = VG_(strtoull10) ((HChar *)token, NULL);
        if (ratio_sqr == 0) return curr;
        #endif
        
        /*
        VG_(printf)("Tuple: %s %llu %llu %llu %llu %llu %llu\n",
                        curr->name, rms, min, max, sum, sqr_sum, occ);
        */
        
        RoutineInfo * rtn_info = HT_lookup(tdata->routine_hash_table, (UWord)curr);
        if (rtn_info == NULL) {
            
            rtn_info = APROF_(new_routine_info)(tdata, curr, (UWord) curr);
        
        } 
        
        RMSInfo * info_access = HT_lookup(rtn_info->rms_map, rms); 
        if (info_access == NULL) {
            
            info_access = (RMSInfo * ) VG_(calloc)("sms_info", 1, sizeof(RMSInfo));
            #if DEBUG
            AP_ASSERT(info_access != NULL, "rms_info not allocable in function exit");
            #endif
            
            info_access->min_cumulative_time = min;
            info_access->key = rms;
            HT_add_node(rtn_info->rms_map, info_access->key, info_access);
            
        } /*else {
            VG_(printf)("Old tuple: %s %lu %llu %llu %llu %llu %llu\n",
                        curr->name, info_access->key, 
                        info_access->min_cumulative_time, 
                        info_access->max_cumulative_time, 
                        info_access->cumulative_time_sum, 
                        info_access->cumulative_time_sqr_sum, 
                        info_access->calls_number);
        }*/
        
        info_access->cumulative_time_sum += sum;
        info_access->cumulative_sum_sqr += sqr_sum;
        info_access->calls_number += occ;

        if (info_access->max_cumulative_time < max) 
            info_access->max_cumulative_time = max;
    
        if (info_access->min_cumulative_time > min) 
            info_access->min_cumulative_time = min;
        
        info_access->cumul_real_time_sum += cumul_real;
        info_access->self_time_sum += self;
        info_access->self_sum_sqr += self_sqr;
        
        if (info_access->self_time_min > self_min) 
            info_access->self_time_min = self_min;
    
        if (info_access->self_time_max < self_max) 
            info_access->self_time_max = self_max;
        
        #if INPUT_METRIC == RVMS
        info_access->rms_input_sum += ratio;
        info_access->rms_input_sum_sqr += ratio_sqr;
        #endif
        
        /*
        VG_(printf)("Current tuple: %s %lu %llu %llu %llu %llu %llu\n",
                        curr->name, info_access->key, 
                        info_access->min_cumulative_time, 
                        info_access->max_cumulative_time, 
                        info_access->cumulative_time_sum, 
                        info_access->cumulative_time_sqr_sum, 
                        info_access->calls_number);
        */

    } else if (token[0] == 'a') {
        
        token = VG_(strtok)(NULL, "@");
        Char app[1024] = {0};
        while (token != NULL) {
            //VG_(printf)("token: %s\n", token);
            VG_(strcat)((HChar *)app, (HChar *)token);
            VG_(strcat)((HChar *)app, " ");
            token = VG_(strtok)(NULL, "@");
        }
        if (VG_(strlen)((HChar *)app) > 0)
            app[VG_(strlen)((HChar *)app) -1] = '\0';
        
        if (VG_(strcmp)((HChar *)app, (HChar *) VG_(args_the_exename)) != 0) {
            VG_(printf)("Command is #%s# versus #%s#\n", app, (Char *) VG_(args_the_exename));
            VG_(printf)("Different command\n");
            return (void *)1; /* special value */
        }
    
    } else if (token[0] == 'k') {
        
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong sum = VG_(strtoull10) ((HChar *)token, NULL);
        if (sum == 0) return curr;
        
        tdata->other_metric += sum;
        
    } else if (token[0] == 'v') {
        
        token = VG_(strtok)(NULL, "@");
        if (token == NULL) return curr;
        ULong ver = VG_(strtoull10) ((HChar *)token, NULL);
        if (ver != REPORT_VERSION) {
            VG_(printf)("Invalid version\n");
            return (void *)1; /* special value */
        }
        
    } else if (token[0] == 'q') {
        
        // Merge of report with CCT is not supported
        return (void *)1; /* special value */
        
    }
    
    return curr;
}

static Bool merge_report(HChar * report, ThreadData * tdata) {
    
    /* open report */
    HChar * rep = VG_(expand_file_name)("aprof log", report);
    //VG_(printf)("Opening: %s\n", rep);
    SysRes res = VG_(open)((const HChar *)rep, VKI_O_RDONLY,
                                VKI_S_IRUSR|VKI_S_IWUSR);
    VG_(free)(rep);
    Int file = (Int) sr_Res(res);
    AP_ASSERT(file > 0, "Can't read a log file.");
    
    Char buf[4096];
    HChar line[1024];
    Int offset = 0;
    Function * current_routine = NULL;
    
    /* merge tuples */
    while (1) {
        
        Int r = VG_(read)(file, buf, 4096);
        if (r < 0) {
            VG_(printf)("Error when reading %s\n", report);
            return False;
        } else if (r == 0) return True; /* EOF */
        
        Int i = 0;
        for (i = 0; i < r; i++) {
            
            if (buf[i] == '\n') {
                
                line[offset++] = '\0';
                //VG_(printf)("# %s\n", line);
                current_routine = merge_tuple(line, offset,
                                            current_routine, tdata);
                
                /* 
                 * this means that the report has a different command 
                 * OR different report version
                 */
                if (current_routine == (void *)1) {
                    VG_(printf)("No merge\n");
                    VG_(close)(file);
                    return False;
                }
                
                offset = 0;
            
            } else line[offset++] = buf[i]; 
            
            AP_ASSERT(offset < 1024, "Line too long");
        }
        
    }    
    
    line[offset++] = '\0';
    //VG_(printf)("# %s\n", line);
    current_routine = merge_tuple(line, offset, current_routine, tdata);
    
    VG_(close)(file);
    return True;
} 
#endif

static HChar * report_name(HChar * filename_priv, UInt tid, UInt postfix_c) {

    #if REPORT_NAME == 1
    
    if ((APROF_(merge_report_runs) || APROF_(merge_report_threads)) 
        && APROF_(running_threads) > 1) {
        
        VG_(sprintf)(filename_priv, "%d_%u_%d", VG_(getpid)(), 
                                    tid - 1, APROF_(addr_multiple));
        
    } else {
    
        if (tid > 1)
            VG_(sprintf)(filename_priv, "%s_%u", 
                    VG_(basename)(prog_name), tid - 1);
        else
            VG_(sprintf)(filename_priv, "%s", 
                VG_(basename)(prog_name));
    }
    
    #elif REPORT_NAME >= 2
    VG_(sprintf)(filename_priv, "%d_%u_%d", VG_(getpid)(), 
                                    tid - 1, APROF_(addr_multiple));
    #endif

    char postfix[128];
    if (postfix_c > 0) VG_(sprintf)(postfix, "_%u.aprof", postfix_c);
    else VG_(sprintf)(postfix, ".aprof");
    
    VG_(strcat)(filename_priv, postfix);

    return filename_priv;

}


static UInt search_report(HChar ** reports, Bool all_runs) {
    
    HChar * directory = VG_(expand_file_name)("aprof log", "./");
    SysRes r = VG_(open)(directory, VKI_O_RDONLY, VKI_S_IRUSR|VKI_S_IWUSR);
    VG_(free)(directory);
    Int dir = (Int) sr_Res(r);
    AP_ASSERT(dir != -1, "Can't open directory.");
    
    struct vki_dirent * file;
    Char buf[1024] = {0};
    UInt n = 0;

    for ( ; ; ) {
    
        Int res = VG_(getdents)(dir, (struct vki_dirent *) &buf, 1024);
        if (res == -1 || res == 0) {
            break;
        }
        
        Int i = 0;
        for (i = 0; i < res;) {

            file = (struct vki_dirent *) (buf + i);
            //VG_(printf)("File: %s - %d\n", file->d_name, file->d_reclen);
            if (VG_(strcmp)(".aprof", file->d_name + 
                    VG_(strlen)(file->d_name) - 6) == 0) {
                
                if (!all_runs) {
                    
                    HChar pid[10] = {0};
                    VG_(sprintf)(pid, "%d", VG_(getpid)()); 
                    if (VG_(strncmp)(file->d_name, pid,
                        VG_(strlen)(pid)) != 0) {
                        i += file->d_reclen;
                        continue;
                    }
                    
                } 
                
                reports[n++] = VG_(strdup)("report", (HChar *)file->d_name);
                //VG_(printf)("File %s - %d\n", file->d_name, file->d_reclen);
            }
            
            i += file->d_reclen;
        }
    
    }
    
    return n;
}

void APROF_(generate_report)(ThreadData * tdata, ThreadId tid) {
    
    HChar filename_priv[1024] = {0};
    HChar * prog_name = (HChar *) VG_(args_the_exename);
    
    #if CCT == 0
    /* last thread? try to merge... */
    if (APROF_(running_threads) == 1) {
        
        //VG_(printf)("I am the last thread\n");
        
        Int n = 0, j = 0;
        HChar ** reports = VG_(calloc)("report array", sizeof(Char *) * 256, 1);
        
        if (APROF_(merge_report_runs))
            n = search_report(reports, True);
        else if (APROF_(merge_report_threads)) 
            n = search_report(reports, False);
        
        for (j = 0; j < n; j++) {
            
            //VG_(printf)("Merging report: %s\n", reports[j]);
            Bool m = merge_report(reports[j], tdata);
            if (m) {
                
                HChar * old = VG_(expand_file_name)("aprof log", reports[j]);
                
                /*
                HChar name[1024];
                VG_(sprintf)(name, "%s_merged", reports[j]);
                HChar * new = VG_(expand_file_name)("aprof log", name);
                VG_(rename) (old, new);
                */
                //VG_(printf)("Renaming report %s -> %s\n", reports[j], name);
                
                VG_(unlink) (old);
                VG_(free)(old);
            }
        }
        
        VG_(free)(reports);
    
    }
    #endif
    
    /*
     * This does not work because we don't have the real path
    if (APROF_(binary_time) == 0) {
        
        // Get binary time
        struct vg_stat buf;
        SysRes b = VG_(stat)(prog_name, &buf);
        binary_time = buf.mtime;
        
    }
    */
    
    /* Add path to log filename */
    HChar * filename = VG_(expand_file_name)("aprof log", 
                            report_name(filename_priv, tid, 0));

    // open report file
    FILE * report = APROF_(fopen)(filename);
    UInt attempt = 0;
    while (report == NULL && attempt < 32) {

        VG_(free)(filename);
        filename = VG_(expand_file_name)("aprof log", 
            report_name(filename_priv, tid, attempt));
        
        report = APROF_(fopen)(filename);
        
        attempt++;
    }

    VG_(free)(filename);
    AP_ASSERT(report != NULL, "Can't create report file");
    
    //VG_(printf)("Writing report TID=%u file=%s\n", tid, filename);

    // write header
    APROF_(fprintf)(report, "c -------------------------------------\n");
    APROF_(fprintf)(report, "c report generated by aprof (valgrind) \n");
    APROF_(fprintf)(report, "c -------------------------------------\n");
    
    // write version 
    APROF_(fprintf)(report, "v %d\n", REPORT_VERSION);
    
    // Maximum value for the metric
    #if TIME == BB_COUNT
    APROF_(fprintf)(report, "k %llu\n", tdata->bb_c + tdata->other_metric);
    #elif TIME == RDTSC
    APROF_(fprintf)(report, "k %llu\n", APROF_(time)() - tdata->entry_time + tdata->other_metric);
    #endif
    
    // write mtime binary
    APROF_(fprintf)(report, "e %llu\n", APROF_(binary_time));
    
    // write performance metric type
    #if TIME == BB_COUNT
    APROF_(fprintf)(report, "m bb-count\n");
    #elif TIME == RDTSC
    APROF_(fprintf)(report, "m time-usec\n");
    #endif
    
    // write input metric type
    #if INPUT_METRIC == RMS
    APROF_(fprintf)(report, "i rms\n");
    #else 
    APROF_(fprintf)(report, "i rvms\n");
    #endif
    
    #if EVENTCOUNT
    APROF_(fprintf)(report, "c JSR=%llu - RTS=%llu - RD=%llu - WR=%llu\n", 
            tdata->num_func_enter, tdata->num_func_exit, 
            tdata->num_read + tdata->num_modify,
            tdata->num_write + tdata->num_modify);
    #endif
    
    // write application name
    APROF_(fprintf)(report, "a %s\n", prog_name);
    
    // write commandline
    APROF_(fprintf)(report, "f %s", prog_name);
    XArray * x = VG_(args_for_client);
    int i = 0;
    for (i = 0; i < VG_(sizeXA)(x); i++) {
        HChar ** c = VG_(indexXA)(x, i);
        if (c != NULL) {
            APROF_(fprintf)(report, " %s", *c);
        }
    }
    APROF_(fprintf)(report, "\n");

    #if DEBUG
    AP_ASSERT(tdata->routine_hash_table != NULL, "Invalid rtn ht");
    #endif

    // iterate over routines
    HT_ResetIter(tdata->routine_hash_table);

    RoutineInfo * rtn_info = HT_RemoveNext(tdata->routine_hash_table);
    while (rtn_info != NULL) {
        
        #if DEBUG
        AP_ASSERT(rtn_info != NULL, "Invalid rtn");
        AP_ASSERT(rtn_info->fn != NULL, "Invalid fn");
        AP_ASSERT(rtn_info->fn->name != NULL, "Invalid name fn");
        #endif
        
        #if DISCARD_UNKNOWN
        if (!rtn_info->fn->discard_info) {
        #endif
        
        char * obj_name = "NONE";
        if (rtn_info->fn->obj != NULL) obj_name = rtn_info->fn->obj->name; 
        
        #if DISTINCT_RMS
        APROF_(fprintf)(report, "r \"%s\" \"%s\" %llu %lu\n", 
                    rtn_info->fn->name, obj_name, 
                    rtn_info->routine_id, 
                    (UInt) HT_count_nodes(rtn_info->distinct_rms));
        #else
        APROF_(fprintf)(report, "r \"%s\" \"%s\" %llu\n", rtn_info->fn->name, 
                    obj_name, rtn_info->routine_id);
        #endif
        
        if (rtn_info->fn->mangled != NULL) {
            APROF_(fprintf)(report, "u %llu \"%s\"\n", rtn_info->routine_id, 
                            rtn_info->fn->mangled);
        }

        #if CCT

        HT_ResetIter(rtn_info->context_rms_map);
        HashTable * ht = HT_RemoveNext(rtn_info->context_rms_map);
        
        while (ht != NULL) {
            
            HT_ResetIter(ht);
            RMSInfo * info_access = HT_RemoveNext(ht);
            
            while (info_access != NULL) {
                
                APROF_(fprintf)(report,
                                #if INPUT_METRIC == RVMS
                                "q %lu %lu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
                                #else
                                "q %lu %lu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
                                #endif
                                ht->key, 
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
                                #if INPUT_METRIC == RVMS
                                ,
                                info_access->rms_input_sum,
                                info_access->rms_input_sum_sqr
                                #endif
                            );

                VG_(free)(info_access);
                info_access = HT_RemoveNext(ht);

            }
            
            HT_destruct(ht);
            ht = HT_RemoveNext(rtn_info->context_rms_map);
        }
        #else

        // iterate over rms records of current routine
        HT_ResetIter(rtn_info->rms_map);
        RMSInfo * info_access = HT_RemoveNext(rtn_info->rms_map);
        
        while (info_access != NULL) {
            
            APROF_(fprintf)(report,
                            #if INPUT_METRIC == RVMS
                            "p %llu %lu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n", 
                            #else
                            "p %llu %lu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
                            #endif
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
                            #if INPUT_METRIC == RVMS
                            ,
                            info_access->rms_input_sum,
                            info_access->rms_input_sum_sqr
                            #endif
                            );

            VG_(free)(info_access);
            info_access = HT_RemoveNext(rtn_info->rms_map);
        }
        #endif
        
        #if DISCARD_UNKNOWN
        }
        #endif

        APROF_(destroy_routine_info)(rtn_info);
        rtn_info = HT_RemoveNext(tdata->routine_hash_table);

    }
    
    #if CCT
    APROF_(print_report_CCT)(report, tdata->root, 0);
    #endif
    
    #if CCT_GRAPHIC
    VG_(sprintf)(filename_priv, "%s_%u.graph", VG_(basename)(prog_name), tid - 1);
    filename = VG_(expand_file_name)("aprof log", filename_priv);
    FILE * cct_rep = APROF_(fopen)(filename);
    AP_ASSERT(cct_rep != NULL, "Can't create CCT report file");
    APROF_(fprintf)(report, "digraph G {\n");
    APROF_(print_cct_graph)(cct_rep, tdata->root, 0, NULL);
    APROF_(fprintf)(report, "}\n");
    APROF_(fclose)(cct_rep);
    VG_(free)(filename);
    #endif

    // close report file
    APROF_(fclose)(report);
    
}
