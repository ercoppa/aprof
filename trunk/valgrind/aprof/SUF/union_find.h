/*
 * Header of Union Find based on forest
 * 
 * Last changed: $Date: 2011-07-08 00:14:45 +0200 (Fri, 08 Jul 2011) $
 * Revision:     $Rev: 70 $
 */

#ifndef _UNIONFIND_H_
#define _UNIONFIND_H_

#ifdef GLIBC
#include <stdlib.h>
#include <stdio.h>

#ifdef GLIB
#include <glib.h>
#endif

#endif

#define ADDRINT unsigned long

typedef struct Node Node;
typedef struct Representative Representative;

struct Representative {
	unsigned int rank;			// Height of the tree
	Representative * next;		// Next representive
	Node * tree;				// The tree
	unsigned int real_nodes;	// # real nodes in the tree
	unsigned int dummies;		// # dummies nodes in the tree
	int stack_depth;			// stack depth associated with this rep
};

struct Node {
	ADDRINT addr;			// (payload) an accessed address
	Node * next;			// Circular linked list of all nodes in the tree
	Node * parent;			// If this node is the root (see IS_ROOT macro), its parent is the rep
};

typedef struct {
	Representative * headRep;	// Last (created) rep
	void * ht;					// HashTable of < address, node * >
	void * pool;				// Allocator pool for nodes/reps
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
//
void UF_insert(UnionFind * uf, ADDRINT addr, int stack_depth);

// ---------------------------------------------------------------------
// UF_lookup
// ---------------------------------------------------------------------
// Find the stack depth of the tree of an accessed address
//		UnionFind * uf: address of the union find
//		ADDRINT addr: accessed address (payload)
// Return -1 if the addr is not in UF
//
int UF_lookup(UnionFind * uf, ADDRINT addr);

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

/* Debug routines */
void UF_print(UnionFind * uf);
void UF_print_tree(Node * node, int level, int reals, int dummies);
void UF_print_count(UnionFind * uf);

#endif
