#ifndef HELPER_DTYPES_H
#define HELPER_DTYPES_H

/*
 * aprof (based on Valgrind which uses a embedded libc) and aprof-tool 
 * (default libc) share some code. This is a "bridge" between the
 * two worlds.
 */

#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h> 

#define INPUT_STATS     1
#define SLOT            2
#define REPORT_VERSION  6
#define DEBUG           1

#define EMSG(...) fprintf(stderr, __VA_ARGS__);

#define BG_GREEN   "\x1b[42m"
#define BG_YELLOW  "\x1b[43m"
#define BG_RESET   "\x1b[0m"
#define BG_RED     "\x1b[41m"

#define YELLOW(str) BG_YELLOW str BG_RESET
#define GREEN(str) BG_GREEN str BG_RESET
#define RED(str) BG_RED str BG_RESET

#define vgAprof_assert(cond, ...)   do { \
                                    if (!(cond)) { \
                                        EMSG(RED("Fatal error:")" "); \
                                        EMSG(__VA_ARGS__); \
                                        EMSG("\n"); \
                                        assert(cond); \
                                    } \
                            } while(0);

#if DEBUG
    #define vgAprof_debug_assert(cond, ...) vgAprof_assert(cond, __VA_ARGS__)
#else
    #define vgAprof_debug_assert(cond, ...)
#endif

struct linux_dirent {
   long           d_ino;
   off_t          d_off;
   unsigned short d_reclen;
   char           d_name[];
};

#define VG_(a)              a
#define calloc(a,b,c)       calloc(b,c)
#define realloc(a,b,c)      realloc(b,c)
#define strdup(a,b)         strdup(b)
#define tl_assert           assert
#define strdup2(a,b)        strdup(b) 
#define strtoull10(a, b)    strtoll(a, b, 10);
#define sr_Res(a)           a
#define sr_isError(a)       (a < 0)
#define umsg                EMSG 

#define getdents64(...)     syscall(SYS_getdents, __VA_ARGS__)
#define vki_dirent64        linux_dirent

#define VKI_O_RDONLY        O_RDONLY 
#define VKI_S_IRUSR         S_IRUSR
#define VKI_S_IWUSR         S_IWUSR  

typedef  unsigned int             UInt;
typedef    signed int             Int;
typedef  unsigned long long int   ULong;
typedef           char            HChar;
typedef  unsigned long            UWord;
typedef           UWord           SizeT;
typedef  unsigned char            Bool;

#define  True   ((Bool)1)
#define  False  ((Bool)0)

typedef           int             SysRes;

#define APROF_(str) vgAprof_##str
#define vgAprof_new(type, size)     calloc(type, size, 1) 
#define vgAprof_delete(type, ptr)   free(ptr) 
#define vgAprof_fprintf             fprintf
#define vgAprof_fclose              fclose

#include "../data-common.h"

void post_merge_consistency(Runtime * r, HChar * report);
UInt APROF_(str_hash)(const HChar *s);
RoutineInfo * APROF_(new_routine_info)(HashTable * rtn_ht, 
                                Function * fn, UWord target);
void APROF_(destroy_routine_info)(RoutineInfo * ri);
Bool APROF_(merge_report)(HChar * report, Runtime * runtime);
void APROF_(print_report)(FILE * report, Runtime * r,
                            HashTable * rtn_ht, ULong cost,
                            CCTNode * root);
UInt APROF_(search_reports)(HChar *** reports, const HChar * path);
void APROF_(destroy_function)(void * fnt);
void APROF_(destroy_object)(void * obj);

#define STR(buf, ...) sprintf(buf, __VA_ARGS__);

#define DIR_MERGE_THREAD "merge_by_pid"
#define DIR_MERGE_RUNS   "merge_by_cmd"
#define DIR_MERGE_BOTH   "merge_by_pid_cmd"
#define DIR_MERGE_ALL    "merge_all"

// check for overflow (long)
#define ADD(dest, inc) do { \
                            ULong old = dest; \
                            dest += inc; \
                            APROF_(assert)(dest >= old, "overflow"); \
                            } while(0); 

// check for overflow (double)
#define ADD_D(dest, inc) do { \
                            double old = dest; \
                            dest += inc; \
                            APROF_(assert)(dest >= old, "overflow"); \
                            } while(0);

#endif
