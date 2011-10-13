#ifndef _SUF_H_
#define _SUF_H_

#ifdef __i386__
#define SPM_SIZE 65536 // 4GB
#else
#define SPM_SIZE 65536 * 8 * 2 // 64GB address space
#endif

#if ADDR_MULTIPLE == 1
#define SSM_SIZE 65536
#elif ADDR_MULTIPLE == 4
#define SSM_SIZE 16384
#else
#error "ADDR_MULTIPLE nor supported"
#endif

typedef struct SSM {
	UInt table[SSM_SIZE];
} SSM;

typedef struct SPM {
	SSM * table[SPM_SIZE];
} StackUF;

StackUF * SUF_create(void);
void SUF_destroy(StackUF * uf);
UInt SUF_insert(StackUF * uf, UWord addr, UInt rid);
UInt SUF_lookup(StackUF * uf, UWord addr);
void SUF_compress(StackUF * uf, UInt * arr_rid, UInt size_arr);
//void SUF_print(StackUF * uf);

#endif
