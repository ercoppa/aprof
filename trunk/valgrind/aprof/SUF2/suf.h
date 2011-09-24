#ifndef _SUF_H_
#define _SUF_H_

#include "pub_tool_basics.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "valgrind.h"

extern void failure(char * msg);

#ifdef __i386__
#define SPM_SIZE 65536 // 4GB
#else
#define SOM_SIZE 65536 * 8 // 32GB address space
#endif

#ifndef ADDR_MULTIPLE
#define ADDR_MULTIPLE 1
#endif

#if ADDR_MULTIPLE == 1
#define SSM_SIZE 65536
#elif ADDR_MULTIPLE == 4
#define SSM_SIZE 16384
#else
#error "ADDR_MULTIPLE nor supported"
#endif

typedef struct SSM {
	UWord table[SSM_SIZE];
} SSM;

typedef struct SPM {
	SSM * table[SPM_SIZE];
} StackUF;

StackUF * SUF_create(void);
void SUF_destroy(StackUF * uf);
UWord SUF_insert(StackUF * uf, UWord addr, UWord rid);
UWord SUF_lookup(StackUF * uf, UWord addr);

#endif
