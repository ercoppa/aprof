#ifndef HELPER_DTYPES_H
#define HELPER_DTYPES_H

#include <stdlib.h>
#include <assert.h>

#define VG_(a) a
#define calloc(a,b,c) calloc(b,c)
#define tl_assert assert
#define strdup2(a,b) strdup(b) 
#define strtoull10(a, b) strtoll(a, b, 10);

typedef  unsigned int             UInt;
typedef    signed int             Int;
typedef  unsigned long long int   ULong;
typedef           char            HChar;
typedef  unsigned long            UWord;
typedef           UWord           SizeT;
typedef  unsigned char            Bool;

#define  True   ((Bool)1)
#define  False  ((Bool)0)

#endif
