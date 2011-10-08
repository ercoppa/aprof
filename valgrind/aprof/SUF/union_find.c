/*
 * Union Find based on forest
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "../aprof.h"
#include "union_find.h"

#define PAGE_NODES 1024

/* Macro for dealing with bit stealing */
#define IS_ROOT(e)          ((UWord)((e)->parent) & 1)
#define SET_AS_ROOT(r, rep) (r)->parent = (void *)(((UWord)((r)->parent) & 2) | ((UWord)(rep) | 1));
#define GET_REP(n)          ((Representative *)((UWord)(n)->parent & ~3))
#define SET_AS_DUMMY(n)     (n)->parent = (void *)((UWord)((n)->parent) | 2);
#define IS_DUMMY(n)         ((UWord)((n)->parent) & 2)
#define SET_PARENT(n, p)    (n)->parent = (void *)(((UWord)((n)->parent) & 2) | (UWord)(p));
#define GET_PARENT(n)       (void *)((UWord)(n)->parent & ~3)

#define CHECK_ADDR_OVERFLOW(x) do { \
									if ((x) > (UWord) UPM_SIZE * 65536) \
										AP_ASSERT(0, "Address overflow"); \
									} while (0);

// ---------------------------------------------------------------------
// UF_find
// ---------------------------------------------------------------------
// Find the tree's root of a node
//		Node * node: a node inserted in a tree
//
static Node * UF_find(Node * n) {

	if (n == NULL) return NULL;
	if (IS_ROOT(n)) return n;
	
	SET_PARENT(n, UF_find(GET_PARENT(n))); /* Path Compression */

	return GET_PARENT(n);

}

/* public functions */

UnionFind * UF_create() {

	UnionFind * uf = (UnionFind *) VG_(calloc)("uf", 1, sizeof(UnionFind));
	AP_ASSERT(uf != NULL, "UF not allocable");
	
	uf->pool = pool_init(PAGE_NODES, sizeof(Node), &uf->free_list);
	AP_ASSERT(uf->pool != NULL, "UF pool not allocable");
	
	uf->free_list = NULL;

	return uf;

}

void UF_destroy(UnionFind * uf){

	int i = 0;
	while (i < UPM_SIZE) {
		if (uf->table[i] != NULL) 
			VG_(free)(uf->table[i]);
		i++;
	}
	
	pool_cleanup(uf->pool);
	VG_(free)(uf);

}

int UF_insert(UnionFind * uf, UWord addr, int stack_depth){

	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	CHECK_ADDR_OVERFLOW(addr);
	#endif

	Node * new = NULL;
	Node * n   = NULL;
	int depth = -1;

	UWord i = addr >> 16;
	UWord j = (addr & 0xffff) / 4;
	if (uf->table[i] != NULL)
		n = uf->table[i]->node[j];
	else {
		uf->table[i] = VG_(calloc)("usm", sizeof(USM), 1);
		AP_ASSERT(uf->table[i] != NULL, "UF sm not allocable");
	}
	
	if (n != NULL) {

		Node * node = UF_find(n);
		
		#ifdef DEBUG
		AP_ASSERT(node != NULL, "Invalid parent");
		#endif
		
		Representative * r = GET_REP(node);
		
		#ifdef DEBUG
		AP_ASSERT(r != NULL, "Invalid rep");
		#endif
		
		if (stack_depth == r->stack_depth)
			return stack_depth;
		
		depth = r->stack_depth;
		r->real_nodes--;
		r->dummies++;
		
		SET_AS_DUMMY(n);
		
		#if DEBUG
		n->addr = 0;
		#endif
		
		pool_alloc(uf->pool, uf->free_list, new, Node);
		AP_ASSERT(new != NULL, "Impossible allocate a node");
		new->parent = NULL;
		new->next = NULL;
		
		uf->table[i]->node[j] = new;
		
		if (r->real_nodes < r->dummies)
			UF_rebalance(uf, r->tree);

	} else {
		
		pool_alloc(uf->pool, uf->free_list, new, Node);
		AP_ASSERT(new != NULL, "Impossible allocate a node");
		new->parent = NULL;
		new->next = NULL;
		
		uf->table[i]->node[j] = new;
		
	}
	
	#if DEBUG
	new->addr = addr;
	#endif

	/* Create new representative and insert the new node */
	if (uf->headRep == NULL || stack_depth != uf->headRep->stack_depth) {

		Representative * new_rep = VG_(calloc)("rep", sizeof(Representative), 1);
		AP_ASSERT(new_rep != NULL, "Impossible allocate a new rep");

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
		AP_ASSERT(IS_ROOT(new), "bad macro");
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

	SET_PARENT(dead->tree, live->tree);

	/* Merge the two node lists */
	Node * head = live->tree->next;
	live->tree->next = dead->tree->next;
	dead->tree->next = head;
	
	if (live->rank == dead->rank) live->rank++;
	live->real_nodes += dead->real_nodes;
	live->dummies    += dead->dummies;
	live->next = uf->headRep->next->next;
	uf->headRep = live;

	VG_(free)(dead);

	return uf->headRep->real_nodes;

}

void UF_rebalance(UnionFind * uf, Node * root) {

	if (root == NULL) return;

	Node * head = root;
	Representative * r = GET_REP(root);
	#ifdef DEBUG
	AP_ASSERT(r != NULL, "Invalid rep during rebalancing");
	#endif
	
	if (r->dummies == 0) return;
	
	r->dummies = 0;
	if (IS_DUMMY(root)) { /* Find a new root */
	
		Node * cand = root->next;
		pool_free(root, uf->free_list);
		root = NULL;
		
		while (cand != NULL && cand != head) {
			
			root = cand;
			if (!IS_DUMMY(root)) break; 
			
			cand = cand->next;
			pool_free(root, uf->free_list); /* We delete all visited dummies */
		}
		
		#ifdef DEBUG
		AP_ASSERT(root != NULL, "invalid root");
		AP_ASSERT(cand != head, "We need at least one real node!");
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
			pool_free(node, uf->free_list);
			
		}
		
	}
	
	if (root->next == NULL) r->rank = 0;
	else r->rank = 1;

}

int UF_lookup(UnionFind * uf, UWord addr){
	
	#if !defined(__i386__) && CHECK_SUF_OVERFLOW
	CHECK_ADDR_OVERFLOW(addr);
	#endif

	UWord i = addr >> 16;
	if (uf->table[i] == NULL) return -1;
	
	UWord j = (addr & 0xffff) / 4;
	Node * n = uf->table[i]->node[j];
	
	if (n == NULL) return -1;

	return GET_REP(UF_find(n))->stack_depth;

}

#if DEBUG
void UF_print(UnionFind * uf) {

	VG_(printf)("\n--------------\n");
	VG_(printf)("| Union find |\n");
	VG_(printf)("--------------\n");
	
	Representative * r = uf->headRep;
	while (r != NULL) {
		VG_(printf)("REP: dummies(%lu) - real(%lu) - rank(%lu) - depth(%u) - addr(%lu)\n",
				r->dummies, r->real_nodes, r->rank, r->stack_depth, (UWord) r);
		if (r->tree == NULL) VG_(printf)("Root is NULL\n");
		if (!IS_ROOT(r->tree)) VG_(printf)("Root not set as root\n");
		if (GET_REP(r->tree) != (void*)r) VG_(printf)("Rep & root are not linked\n");
		UF_print_tree(r->tree, 0, r->real_nodes, r->dummies);
		r = r->next;
	}
}

void UF_print_tree(Node * node, int level, int reals, int dummies) {

	if (node == NULL) return;
	
	int real_nodes = 0;
	int dummy_nodes = 0;
	
	Node * head = node;
	
	while (node != NULL) {
		
		if(IS_ROOT(node)) VG_(printf)("[0] ");
		else VG_(printf)("[1] ");
		VG_(printf)("Addr(%lu)\n", node->addr);
		
		AP_ASSERT(node->addr == 0 && IS_DUMMY(node), "Dummy not marked as dummy");
		
		if (IS_DUMMY(node)) dummy_nodes++;
		else real_nodes++;
		
		if (node->next == head) {
			VG_(printf)("Linked correctly to head\n");
			break;
		}
		
		node = node->next;
		
	}
	
	if (real_nodes != reals) VG_(printf)("Wrong # real nodes\n");
	if (dummy_nodes != dummies) VG_(printf)("Wrong # dummy nodes\n");
	
}
#endif
