/*
 * aprof-helper
 */

#include <stdio.h>
#include "aprof-helper.h"
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

static void fn_destroy(void * fnt) {
    Function * fn = (Function *) fnt;
    VG_(free)(fn->name);
    VG_(free)(fn->mangled);
    HT_destruct(fn->input_map);
    VG_(free)(fnt);
}

static void obj_destroy(void * obj) {
    Object * o = (Object *) obj;
    VG_(free)(o->name);
    VG_(free)(o);
}

void post_merge_consistency(Runtime * r, HChar * report) {
    
    /* ToDo: re-add some sanity checks */
    return;
    
    ULong tuples = 0;
    
    HT_ResetIter(r->fn_ht);
    Function * fn = (Function *) HT_Next(r->fn_ht);
    while (fn != NULL) {

        HT_ResetIter(fn->input_map);
        Input * i = (Input *) HT_Next(fn->input_map);
        while (i != NULL) {
            
            tuples++;
            i = (Input *) HT_Next(fn->input_map);
        }

        fn = (Function *) HT_Next(r->fn_ht);
    }
}

static void clean_data(Runtime * rep) {
    
    APROF_(assert)(rep != NULL, "Invalid aprof report");
    
    if (rep->rtn_ht != NULL)
        HT_destruct(rep->rtn_ht);
    if (rep->fn_ht != NULL)
        HT_destruct(rep->fn_ht);
    if (rep->obj_ht != NULL)
        HT_destruct(rep->obj_ht);
    if (rep->cmd_line != NULL)
        VG_(free)(rep->cmd_line);
    if (rep->application != NULL)
        VG_(free)(rep->application);
}

static void reset_data(Runtime * rep) {
    
    clean_data(rep);
    
    rep->merge_threads = False;       
    rep->sqr_cumul_overflow = False;
    rep->sqr_self_overflow = False;
    rep->total_real_cost = 0;
    rep->total_self_cost = 0;
    rep->total_cumul_thread = 0;
    rep->total_cumul_syscall = 0;
    rep->total_self_thread = 0;
    rep->total_self_syscall = 0;
    rep->next_function_id = 0;
    rep->cmd_line = NULL;
    rep->application = NULL;
    rep->memory_resolution = 0;
    rep->fn_ht = HT_construct(fn_destroy);
    rep->obj_ht = HT_construct(obj_destroy);
    rep->rtn_ht = HT_construct(VG_(free));
    rep->input_metric = INVALID;
}

static Int get_pid_report(HChar * report) {
    
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

static Int get_tid_report(HChar * report) {
    
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
    
    p++;
    Int tid = VG_(strtoull10)(p, NULL);
    
    return tid;
}

static void create_rtn_ht(Runtime * r) {
        
    HT_ResetIter(r->fn_ht);
    Function * f = HT_Next(r->fn_ht);
    while (f != NULL) {
        
        APROF_(new_routine_info)(r->rtn_ht, f, (UWord) f);
        f = HT_Next(r->fn_ht);
    }
}

static HChar ** merge_by_run(HChar ** list, UInt * size, Runtime * r) {
    
    if (r->merge_all)
        printf("Merging all reports...\n");
    else
        printf("Merging reports with same command...\n");
    if (*size == 0)
        return list;
    
    UInt size_post = 0;
    UInt i = 0;
    HChar ** list_post = VG_(calloc)("test", sizeof(HChar *), *size);
    
    /*
     * Merge by command
     */
    char merge_dir[1024] = { 0 };
    int res = 0;
    HChar buf[1024] = { 0 };
    STR(buf, "%s", list[0]);
    if (r->merge_threads)
        res = sprintf(merge_dir, "%s/../%s", dirname(buf), DIR_MERGE_BOTH);
    else if (r->merge_runs)
        res = sprintf(merge_dir, "%s/%s", dirname(buf), DIR_MERGE_RUNS);
    else
        res = sprintf(merge_dir, "%s/%s", dirname(buf), DIR_MERGE_ALL);
    
    APROF_(assert)(res < 1024, "path too long");
    res = mkdir(merge_dir, 0777);
    APROF_(assert)(errno != EEXIST, "directory %s already exists", merge_dir);
    APROF_(assert)(res == 0, "Invalid merge directory");
    
    Int curr = -1;
    UInt merged = 0;
    UInt curr_pid = 0, curr_tid = 0;
    while (1) {
        
        if (list[i] == NULL)
            goto next;
        
        //printf("checking report: %s\n", list[i]);
        
        if (curr == -1) {
            
            curr = i;
            curr_pid = get_pid_report(list[i]);
            curr_tid = get_tid_report(list[i]);
            
            reset_data(r);
            
            //printf("Trying with %s\n", list[curr]);
            Bool res = APROF_(merge_report)(list[curr], r);
            APROF_(assert)(res, "Report %s should be merged but is invalid", list[curr]);
            merged++;
            
        } else {
            
            APROF_(assert)(merged > 0, "Impossible");
            
            /*
             printf("Current: %s [%u:%u] - Checking: %s [%u:%u]\n", 
             list[curr], curr_pid, curr_pid, list[i],
             get_pid_report(list[i]), get_tid_report(list[i]));
             */
            if (r->merge_all
                    || (curr_pid != get_pid_report(list[i])
                            && (r->merge_threads
                                    || curr_tid == get_tid_report(list[i])))) {
                
                /*
                 printf("Current: %s [%u:%u] - Checking: %s [%u:%u]\n", 
                 list[curr], curr_pid, curr_tid, list[i],
                 get_pid_report(list[i]), get_tid_report(list[i]));
                 */

                Bool res = APROF_(merge_report)(list[i], r);
                if (res) {
                    if (merged == 1)
                        printf("Merging: %s %s ", list[curr], list[i]);
                    else
                        printf("%s ", list[i]);
                    free(list[i]);
                    list[i] = NULL;
                    merged++;
                }
            }
            
        }
        
        next: if (i + 1 == *size) {
            
            if (curr == -1)
                break;
            
            if (merged > 0) {
                
                HChar * new_rep = VG_(calloc)("t", 1024, 2);
                HChar buf[1024] = { 0 };
                STR(buf, "%s", list[curr]);
                //printf("saving %s\n", list[curr]);
                if (r->merge_threads)
                    sprintf(new_rep, "%s/../%s/%s", dirname(buf),
                            DIR_MERGE_BOTH, basename(list[curr]));
                else if (r->merge_runs)
                    sprintf(new_rep, "%s/%s/%s", dirname(buf), DIR_MERGE_RUNS,
                            basename(list[curr]));
                else
                    sprintf(new_rep, "%s/%s/%s", dirname(buf), DIR_MERGE_ALL,
                            basename(list[curr]));
                
                if (merged > 1)
                    printf("into "YELLOW("%s")"\n", new_rep);
                
                create_rtn_ht(r);
                
                FILE * report = fopen(new_rep, "w");
                APROF_(assert)(report != NULL, 
                                "Can't save report: %s\n", new_rep);
                
                APROF_(print_report)(report, r, r->rtn_ht, r->extra_cost, NULL);
                                        
                list_post[size_post++] = new_rep;
                
                merged = 0;
                free(list[curr]);
                
            } else {
                
                APROF_(assert)(0, "Impossible");
                
            }
            
            list[curr] = NULL;
            curr = -1;
        }
        
        i = (i + 1) % *size;
    }
    
    i = 0;
    while (i < *size) {
        if (list_post[i] != NULL)
            VG_(free)(list[i]);
        i++;
    }
    VG_(free)(list);
    
    *size = size_post;
    return list_post;
}

static HChar ** merge_by_thread(HChar ** list, UInt * size, Runtime * r) {
    
    printf("Merging reports with same PID...\n");
    if (*size == 0)
        return list;
    
    UInt size_post = 0;
    HChar ** list_post = VG_(calloc)("test", sizeof(HChar *), *size);
    
    /*
     * Merge by PID
     */

    HChar merge_dir[1024] = { 0 };
    HChar buf[1024] = { 0 };
    STR(buf, "%s", list[0]);
    int res = sprintf(merge_dir, "%s/%s", dirname(buf), DIR_MERGE_THREAD);
    APROF_(assert)(res < 1024, "path too long");
    res = mkdir(merge_dir, 0777);
    APROF_(assert)(errno != EEXIST, "directory %s already exists", merge_dir);
    APROF_(assert)(res == 0, "Invalid merge directory");
    
    UInt i = 0;
    Int curr = -1;
    UInt merged = 0;
    UInt curr_pid = 0;
    while (1) {
        
        if (list[i] == NULL)
            goto next;
        
        //printf("checking report: %s\n", list[i]);
        
        if (curr == -1) {
            
            curr = i;
            curr_pid = get_pid_report(list[i]);
            
        } else {
            
            //printf("Current: %s\n", list[curr]);
            if (curr_pid == get_pid_report(list[i])) {
                
                if (merged == 0) {
                    
                    reset_data(r);
                    
                    printf("Merging %s ", list[curr]);
                    Bool res = APROF_(merge_report)(list[curr], r);
                    APROF_(assert)(res,
                            "Report %s should be merged but is invalid", list[curr]);
                    merged++;
                }
                
                printf("%s ", list[i]);
                Bool res = APROF_(merge_report)(list[i], r);
                APROF_(assert)(res,
                        "Report %s should be merged but is invalid", list[i]);
                free(list[i]);
                list[i] = NULL;
                merged++;
            }
            
        }
        
        next: if (i + 1 == *size) {
            
            if (curr == -1)
                break;
            
            HChar * new_rep = VG_(calloc)("t", 1024, 2);
            HChar cmd[1024] = { 0 };
            if (merged > 0) {
                
                HChar buf[1024] = { 0 };
                STR(buf, "%s", list[curr]);
                sprintf(new_rep, "%s/%s/%s", dirname(buf), DIR_MERGE_THREAD,
                        basename(list[curr]));
                
                printf("into "YELLOW("%s")"\n", new_rep);
                //printf("saving %s to %s\n", list[curr], new_rep);
                
                FILE * report = fopen(new_rep, "w");
                APROF_(assert)(report != NULL, 
                                "Can't save report: %s\n", new_rep);
                
                APROF_(print_report)(report, r, r->rtn_ht, r->extra_cost, NULL);
                
                list_post[size_post++] = new_rep;
                
                merged = 0;
                
            } else {
                
                HChar buf[1024] = { 0 };
                STR(buf, "%s", list[curr]);
                sprintf(new_rep, "%s/%s/%s", dirname(buf), DIR_MERGE_THREAD,
                        basename(list[curr]));
                
                //printf("no candidate for merging %s, copying to %s\n", list[curr], new_rep);
                
                list_post[size_post++] = new_rep;
                
                sprintf(cmd, "cp %s %s", list[curr], new_rep);
                //printf("%s\n", cmd);
                res = system(cmd);
                APROF_(assert)(res != -1, "Error during copy");
                
            }
            
            free(list[curr]);
            list[curr] = NULL;
            curr = -1;
        }
        
        i = (i + 1) % *size;
    }
    
    i = 0;
    while (i < *size) {
        if (list_post[i] != NULL)
            VG_(free)(list[i]);
        i++;
    }
    VG_(free)(list);
    
    *size = size_post;
    return list_post;
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
    
    printf("\n  Other options:\n");
    
    printf("    -d <PATH>  report's directory [default: working directory if no -d and -a options specified]\n");
    printf("    -a <PATH>  perform an action on a specific report\n");

    printf("\n");
    return;
}

Int main(Int argc, HChar *argv[]) {
    
    /*
     * Parse options, sanitize options, etc
     */

    if (argc == 1) {
        cmd_options(argv[0]);
        return 1;
    }
    
    Bool consistency = False;
    Bool merge_all = False;
    Bool merge_threads = False;
    Bool merge_runs = False;
    Bool compare = False;
    
    HChar * logs[SLOT] = { NULL, NULL };
    HChar * directory = NULL;
    Runtime report[SLOT];
    memset(&report, 0, sizeof(report));
    
    opterr = 0;
    Int opt;
    UInt i;
    
    while ((opt = getopt(argc, argv, "rtkia:d:")) != -1) {
        
        switch (opt) {
            
            case 'i':
                merge_all = True;
                break;
                
            case 'a':
                logs[0] = optarg;
                if (optarg[0] == '=')
                    logs[0]++;
                break;
                
            case 'b':
                logs[1] = optarg;
                if (optarg[0] == '=')
                    logs[1]++;
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
                if (optarg[0] == '=')
                    directory++;
                APROF_(assert)(strlen(optarg) > 0 && strlen(optarg) < 1024,
                        "path too long");
                HChar dir[1024];
                if (optarg[strlen(optarg) - 1] == '/') {
                    STR(dir, "%s", optarg);
                } else {
                    STR(dir, "%s/", optarg);
                }
                directory = dir;
                //printf("directory := %s\n", directory);
                break;
                
            case '?':
                if (optopt == 'd')
                    fprintf(stderr, "Option -%c requires an argument.\n",
                            optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n",
                            optopt);
                
                return 1;
                
            default:
                abort();
                
        }
        
    }
    
    if (!merge_all && !merge_runs && !merge_threads && !consistency
            && !compare) {
        cmd_options(argv[0]);
        return 1;
    }
    
    if (logs[0] != NULL
            && strcmp(".aprof", logs[0] + strlen(logs[0]) - 6) != 0) {
        
        EMSG("Invalid first report\n");
        return 1;
    }
    
    if (logs[1] != NULL
            && strcmp(".aprof", logs[1] + strlen(logs[1]) - 6) != 0) {
        
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
        
        EMSG("option -i conflicts with -t or -r\n");
        cmd_options(argv[0]);
        return 1;
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
    HChar ** list;
    if (directory != NULL && logs[0] == NULL) {
        
        //printf("Searching list in: %s\n", directory);
        size = APROF_(search_reports)(&list, directory);
        
    } else {
        
        list = logs;
        //printf("Report[0]: %s\n", logs[0]);
        
        if (logs[1] == NULL)
            size = 1;
        else {
            //printf("Report[1]: %s\n", logs[1]);
            size = 2;
        }
        
    }
    
    report[0].consistency = consistency;
    report[0].merge_all = merge_all;
    report[0].merge_runs = merge_runs;
    report[0].merge_threads = merge_threads;
    reset_data(&report[0]);

    report[1].consistency = consistency;
    report[1].merge_all = merge_all;
    report[1].merge_runs = merge_runs;
    report[1].merge_threads = merge_threads;
    reset_data(&report[1]);
    
    /*
     * Do something!
     */

    if (consistency) {
        
        i = 0;
        while (i < size) {
            
            reset_data(&report[0]);
            APROF_(merge_report)(list[i++], &report[0]);
            //create_rtn_ht(&report[0]);
        }
                
        printf(GREEN("Passed:")" ");
        if (size == 1)
            printf("Checked consistency of report: %s\n", list[0]);
        else if (logs[0] != NULL && logs[1] != NULL) {
            printf("Checked consistency of report: %s\n", list[0]);
            printf("Checked consistency of report: %s\n", list[1]);
        } else
            printf("Checked consistency of %d reports in: %s\n", size,
                    dirname(list[0]));
        
    } else { // merge
    
        if (merge_threads)
            list = merge_by_thread(list, &size, &report[0]);
        
        if (merge_all)
            APROF_(assert)(!merge_threads, "Invalid");
        
        if (merge_runs || merge_all)
            list = merge_by_run(list, &size, &report[0]);
        
    }
    
    /*
     * Clean up memory
     */

    for (i = 0; i < SLOT; i++) 
        clean_data(&report[i]);
    
    if (directory != NULL && logs[0] == NULL) {
        
        i = 0;
        while (i < size) {
            if (list[i] != NULL)
                VG_(free)(list[i]);
            i++;
        }
        VG_(free)(list);
        
    }
    
    return 0;
}
