/*
 * Union Find based on forest
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#ifndef GLIBC
#include "pub_tool_basics.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#endif

#include "union_find.h"
#include "../glibc-valgrind.h"

#define DEBUG

/* Macro for dealing with bit stealing */
#define IS_ROOT(e)          ((unsigned long)((e)->parent) & 1)
#define SET_AS_ROOT(r, rep) (r)->parent = (void *)(((unsigned long)((r)->parent) & 2) | ((unsigned long)(rep) | 1));
#define GET_REP(n)          ((Representative *)((unsigned long)(n)->parent & ~3))
#define SET_AS_DUMMY(n)     (n)->parent = (void *)((unsigned long)(n) | 2);
#define IS_DUMMY(n)         ((unsigned long)((n)->parent) & 2)

static void failure(char * c) {
	
	printf("%s\n", c);
	exit(1);
	
}

// ---------------------------------------------------------------------
// UF_find
// ---------------------------------------------------------------------
// Find the tree's root of a node
//		Node * node: a node inserted in a tree
//
static Node * UF_find(Node * n) {

	if (n == NULL) return NULL;
	if (IS_ROOT(n)) return n;
	
	n->parent = UF_find(n->parent); /* Path Compression */

	return n->parent; /* if n is not root, its n->parent it's clean wrt bit stealing */

}

/* public functions */

UnionFind * UF_create() {

	UnionFind * uf = (UnionFind *) calloc(1, sizeof(UnionFind));
	if (uf == NULL) return NULL;

	return uf;

}

void UF_destroy(UnionFind * uf){

	int i = 0;
	while (i < 65536) {
		if (uf->table[i] != NULL) 
			free(uf->table[i]);
		i++;
	}
	free(uf);

}

int UF_insert(UnionFind * uf, ADDRINT addr, int stack_depth){

	Node * new = NULL;
	Node * n   = NULL;
	int depth = 0;

	ADDRINT i = addr >> 16;
	ADDRINT j = (addr & 0xffff) / 4;
	if (uf->table[i] != NULL)
		n = uf->table[i]->node[j];
	else {
		uf->table[i] = calloc(sizeof(USM), 1);
		if (uf->table[i] == NULL) failure("UF sm not allocable");
	}
	
	if (n != NULL) {

		Node * node = UF_find(n);
		
		#ifdef DEBUG
		if (node == NULL) failure("Invalid parent");
		#endif
		
		Representative * r = GET_REP(node);
		
		#ifdef DEBUG
		if (r == NULL) failure("Invalid rep");
		#endif
		
		if (stack_depth == r->stack_depth)
			return stack_depth;
		
		depth = r->stack_depth;
		r->real_nodes--;
		r->dummies++;
		
		SET_AS_DUMMY(n);
		
		new = malloc(sizeof(Node));
		if (new == NULL) failure("Impossible allocate a node");
		
		uf->table[i]->node[j] = new;
		
		if (r->real_nodes < r->dummies)
			UF_rebalance(uf, r->tree);

	} else {
		
		new = malloc(sizeof(Node));
		if (new == NULL) failure("Impossible allocate a node");
		
		uf->table[i]->node[j] = new;
		
	}

	/* Create new representative and insert the new node */
	if (uf->headRep == NULL || stack_depth != uf->headRep->stack_depth) {

		Representative * new_rep = malloc(sizeof(Representative));
		if (new_rep == NULL) failure("Impossible allocate a new rep");
		new_rep->rank = 0;
		new_rep->real_nodes = 1;
		new_rep->dummies = 0;
		new_rep->stack_depth = stack_depth;
		
		/* Connect with the node */
		new_rep->tree = new;
		new->next = new; /* Loop because circular list*/
		
		/* Set the node as root of the tree */
		SET_AS_ROOT(new, new_rep);
		
		#ifdef DEBUG
		if (!IS_ROOT(new)) failure("bad macro\n");
		#endif
		
		/* Insert in the representative list */
		new_rep->next = uf->headRep;
		uf->headRep = new_rep;

		return depth;
	
	}

	/* Insert node into the current tree and update current rep info */
	new->parent = uf->headRep->tree;
	new->next = uf->headRep->tree->next;
	uf->headRep->tree->next = new;
	uf->headRep->real_nodes++;
	if (uf->headRep->rank == 0)
		uf->headRep->rank = 1;
		
	return depth;

}

int UF_merge(UnionFind * uf, int current_stack_depth){

	/* Current (exiting) routine has not accessed any var */
	if (uf->headRep == NULL || uf->headRep->stack_depth != current_stack_depth)
		return 0;

	/* Previous routine has not accessed any var */
	if (uf->headRep->next == NULL || uf->headRep->next->stack_depth != current_stack_depth-1) {
		uf->headRep->stack_depth--;
		return uf->headRep->real_nodes;
	}

	/* Union by rank */
	Representative * live;
	Representative * dead;

	if (uf->headRep->rank >= uf->headRep->next->rank) {
		live = uf->headRep;
		dead = uf->headRep->next;
		live->stack_depth--;
	} else {
		live = uf->headRep->next;
		dead = uf->headRep;
	}

	dead->tree->parent = live->tree; /* Implicitly we clear the last bit! */
	/* Merge the two node lists */
	Node * head = live->tree->next;
	live->tree->next = dead->tree->next;
	dead->tree->next = head;
	
	if (live->rank == dead->rank) live->rank++;
	live->real_nodes += dead->real_nodes;
	live->dummies    += dead->dummies;
	live->next = uf->headRep->next->next;
	uf->headRep = live;

	free(dead);

	return uf->headRep->real_nodes;

}

void UF_rebalance(UnionFind * uf, Node * root) {

	if (root == NULL) return;

	//printf("Rebalancing...\n");

	Node * head = root;
	Representative * r = GET_REP(root);
	#ifdef DEBUG
	if (r == NULL) failure("Invalid rep during rebalancing");
	#endif
	
	r->dummies = 0;
	if (IS_DUMMY(root)) { /* Find a new root */
	
		Node * cand = root->next;
		free(root);
		root = NULL;
		
		while (cand != NULL && cand != head) {
			
			root = cand;
			if (!IS_DUMMY(cand)) break;
			
			free(root); /* We delete all visited dummies */
			cand = cand->next;
		}
		
		#ifdef DEBUG
		if (root == NULL || cand == head)
			failure("We need at least one real node!\n");
		#endif
	
	}
	
	r->tree = root;
	SET_AS_ROOT(root, r);
	
	/* Make the tree a star */
	Node * node;
	Node * next = root->next;
	root->next = NULL;
	while (next != NULL && next != head) {
		
		node = next;
		if (!IS_DUMMY(node)) {
			
			next = node->next;
			node->parent = root;
			if (root->next == NULL) /* This node is new tail */
				node->next = root;
			else
				node->next = root->next;
				
			root->next = node;
			
		} else {
			
			next = node->next;
			free(node);
			
		}
		
	}
	
	if (root->next == NULL) r->rank = 0;
	else r->rank = 1;

}

int UF_lookup(UnionFind * uf, ADDRINT addr){

	ADDRINT i = addr >> 16;
	if (uf->table[i] == NULL) return -1;
	
	ADDRINT j = (addr & 0xffff) / 4;
	Node * n = uf->table[i]->node[j];
	
	if (n == NULL) return -1;

	return GET_REP(UF_find(n))->stack_depth;

}
