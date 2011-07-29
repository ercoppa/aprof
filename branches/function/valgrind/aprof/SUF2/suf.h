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
#define SSM_SIZE 65536 // 4GB
#else
#define SSM_SIZE 65536 * 8 // 32GB address space
#endif

typedef struct SSM {
	UWord table[16384];
} SSM;

typedef struct SPM {
	SSM * table[SSM_SIZE];
} StackUF;

StackUF * SUF_create(void);
void SUF_destroy(StackUF * uf);
UWord SUF_insert(StackUF * uf, UWord addr, UWord rid);
UWord SUF_lookup(StackUF * uf, UWord addr);

#endif
