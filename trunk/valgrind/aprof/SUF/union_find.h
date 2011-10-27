/*
 * Header of Union Find based on forest
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#ifndef _UNIONFIND_H_
#define _UNIONFIND_H_

#ifdef __i386__
#define UPM_SIZE 65536 // 4GB
#else
#define UPM_SIZE 65536 * 8 // 32GB address space
#endif

#if ADDR_MULTIPLE == 4
#define USM_SIZE 16384
#elif SUF == 1
#error "ADDR_MULTIPLE nor supported"
#else 
#define USM_SIZE 16384
#endif

typedef struct Node Node;
typedef struct Representative Representative;

struct Representative {
	UInt rank;					// Height of the tree
	Representative * next;		// Next representive
	Node * tree;				// The tree
	UInt real_nodes;			// # real nodes in the tree
	UInt dummies;				// # dummies nodes in the tree
	int stack_depth;			// stack depth associated with this rep
};

struct Node {
	Node * next;			// Circular linked list of all nodes in the tree
	Node * parent;			// If this node is the root (see IS_ROOT macro), its parent is the rep
	#if DEBUG
	UWord addr;
	#endif
};

typedef struct USM {
	Node * node[USM_SIZE];
} USM;

typedef struct {
	Representative * headRep;	// Last (created) rep
	USM * table[UPM_SIZE];
	pool_t * pool;
	void * free_list;
} UnionFind;

// ---------------------------------------------------------------------
// UF_create
// ---------------------------------------------------------------------
// create new union find data structure
//
UnionFind * UF_create(void);

// ---------------------------------------------------------------------
// UF_destroy
// ---------------------------------------------------------------------
// destroy union find data structure
//		UnionFind * uf: address of the union find
//
void UF_destroy(UnionFind * uf);

// ---------------------------------------------------------------------
// UF_insert
// ---------------------------------------------------------------------
// Insert an address for the stack_depth indicated, if the node already
// exists (in a tree with < stack_depth), the old one it's replaced with  
// a dummy node (addr == 0) and inserted in the current tree (eventually
// a new tree is created).
//		UnionFind * uf: address of the union find
//		ADDRINT addr: accessed address (payload)
//		int stack_depth: stack depth of the routine that access addr
// Return the stack depth of the old representative (if the address is
// already present in the UF. 
//
int UF_insert(UnionFind * uf, UWord addr, int stack_depth);

// ---------------------------------------------------------------------
// UF_lookup
// ---------------------------------------------------------------------
// Find the stack depth of the tree of an accessed address
//		UnionFind * uf: address of the union find
//		ADDRINT addr: accessed address (payload)
// Return -1 if the addr is not in UF
//
int UF_lookup(UnionFind * uf, UWord addr);

// ---------------------------------------------------------------------
// UF_merge
// ---------------------------------------------------------------------
// Merge the tree with stack_depth and the tree with (stack_depth-1) 
// (only if they are the last two created tree).
//		UnionFind * uf: address of the union find
//		ADDRINT addr: accessed address (payload)
// Return the number of (real) nodes in the merged tree, else 0
//
int UF_merge(UnionFind * uf, int current_stack_depth);

// ---------------------------------------------------------------------
// UF_rebalance
// ---------------------------------------------------------------------
// Reorganize a tree (delete dummies, make a star structure)
//		UnionFind * uf: address of the union find
//		Node * node: a node of the tree
//
void UF_rebalance(UnionFind * uf, Node * root);

#if DEBUG
void UF_print_tree(Node * node, int level, int reals, int dummies);
void UF_print(UnionFind * uf);
#endif



#endif
