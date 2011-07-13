/*
 * Union Find based on forest
 * 
 * Last changed: $Date: 2011-07-09 16:20:22 +0200 (Sat, 09 Jul 2011) $
 * Revision:     $Rev: 74 $
 */

/* FixMe
 * Check valori di ritorno calloc, uf_alloc_*, ghash_table_*
 */

/* Idee/dubbi:
 * - Capire se un address e' stato gia' acceduto dalla stessa funzione,
 *   richiede il confronto tra la stackdepth attuale (passata ad UF_insert)
 *   e la stackdepth del rappresentante dell'albero in cui si trova
 *   l'eventuale nodo associato ad address. 
 *   Quindi occorre invocare UF_find(). Per migliorare
 *   le prestazioni in tempo si potrebbe assocciare al singolo nodo
 *   l'info sulla stackdepth (e non al suo rappresentante) ovviamente
 *   pagando in spazio...
 * - UF_lookup() e UF_insert() sono spesso chiamate in modo accoppiato (aprof PIN),
 *   ma ognuna di essa effettua una lookup del node nella ht, possiamo
 *   ottimizzare?
 */

#include "union_find.h"
#include "uf_alloc.h"
#include "valgrind.h"

#define DEBUG

#ifndef GLIB
#include "hashtable.h"
#endif

/* Macro for dealing with bit stealing */
#define IS_ROOT(e)          ((unsigned long)((e)->parent) & 1)
#define SET_AS_ROOT(r, rep) do { (r)->parent = (void *)((unsigned long)(rep) | 1); } while(0);
#define GET_REP(n)          ((Representative *)((unsigned long)(n)->parent & ~1))

/* private functions */

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
	
	#ifdef GLIB
	uf->ht = g_hash_table_new(g_direct_hash, g_direct_equal);
	#else
	uf->ht = HT_construct(NULL);
	#endif
	
	uf->pool = uf_alloc_init();

	if (uf->ht == NULL) failure("No alloc ht");
	if (uf->pool == NULL) failure("No pool"); 

	return uf;

}

void UF_destroy(UnionFind * uf){

	#ifdef GLIB
	g_hash_table_destroy(uf->ht);
	#else
	HT_destruct(uf->ht);
	#endif
	
	uf_alloc_destroy(uf->pool);
	return;

}

void UF_insert(UnionFind * uf, ADDRINT addr, int stack_depth){

	Node * new = NULL;

	#ifdef GLIB
	Node * n = (Node *) g_hash_table_lookup(uf->ht, (void *) addr);
	#else
	HashNode * hn;
	Node * n = HT_lookup(uf->ht, addr, &hn);
	#endif
	
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
			return;
		
		r->real_nodes--;
		r->dummies++;
		n->addr = 0;
		
		new = uf_alloc_node(uf->pool);
		if (new == NULL) failure("Impossible allocate a node");
		
		#ifdef GLIB
		g_hash_table_replace(uf->ht, (void *) addr, new);
		#else
		hn->value = new;
		#endif
		
		if (r->real_nodes < r->dummies)
			UF_rebalance(uf, r->tree);

	} else {
		
		new = uf_alloc_node(uf->pool);
		if (new == NULL) failure("Impossible allocate a node");
		
		#ifdef GLIB
		g_hash_table_insert(uf->ht, (void *) addr, new);
		#else
		HT_add_node (uf->ht, addr, new);
		#endif
	}
	
	new->addr = addr;

	/* Create new representative and insert the new node */
	if (uf->headRep == NULL || stack_depth != uf->headRep->stack_depth) {

		Representative * new_rep = uf_alloc_rep(uf->pool);
		if (new_rep == NULL) failure("Impossible allocate a new rep");
		new_rep->rank = 0;
		new_rep->real_nodes = 1;
		new_rep->dummies = 0;
		new_rep->stack_depth = stack_depth;
		
		/* Connect with the node */
		new_rep->tree = new;
		new->next = new; /* Loop */
		
		/* Set the node as root of the tree */
		SET_AS_ROOT(new, new_rep);
		
		#ifdef DEBUG
		if (!IS_ROOT(new)) failure("bad macro\n");
		#endif
		
		/* Insert in the representative list */
		new_rep->next = uf->headRep;
		uf->headRep = new_rep;

		return;
	
	}

	/* Insert node into the current tree and update current rep info */
	new->parent = uf->headRep->tree;
	new->next = uf->headRep->tree->next;
	uf->headRep->tree->next = new;
	uf->headRep->real_nodes++;
	if (uf->headRep->rank == 0)
		uf->headRep->rank = 1;

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

	uf_free_rep(uf->pool, dead);

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
	if (root->addr == 0) { /* Find a new root */
	
		Node * cand = root->next;
		uf_free_node(uf->pool, root);
		root = NULL;
		
		while (cand != NULL && cand != head) {
			
			root = cand;
			if (root->addr != 0) break;
			
			uf_free_node(uf->pool, root); /* We delete all visited dummies */
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
		if (node->addr != 0) {
			
			next = node->next;
			node->parent = root;
			if (root->next == NULL) /* This node is new tail */
				node->next = root;
			else
				node->next = root->next;
				
			root->next = node;
			
		} else {
			
			next = node->next;
			uf_free_node(uf->pool, node);
			
		}
		
	}
	
	if (root->next == NULL) r->rank = 0;
	else r->rank = 1;

}

int UF_lookup(UnionFind * uf, ADDRINT addr){

	#ifdef GLIB
	Node * n = (Node *) g_hash_table_lookup(uf->ht, (void *) addr);
	#else
	Node * n = HT_lookup (uf->ht, addr, NULL);
	#endif
	
	if (n == NULL) return -1;

	return GET_REP(UF_find(n))->stack_depth;

}

#ifdef DEBUG

void UF_print_count(UnionFind * uf) {
	Representative * r = uf->headRep;
	while (r != NULL) {
		printf("REP: dummies(%u) - real(%u) - rank(%u) - depth(%u)\n",
				r->dummies, r->real_nodes, r->rank, r->stack_depth);
		r = r->next;
	}
}

void UF_print(UnionFind * uf) {

	printf("\n--------------\n");
	printf("| Union find |\n");
	printf("--------------\n");
	
	Representative * r = uf->headRep;
	while (r != NULL) {
		printf("REP: dummies(%u) - real(%u) - rank(%u) - depth(%u) - addr(%lu)\n",
				r->dummies, r->real_nodes, r->rank, r->stack_depth, (unsigned long int) r);
		if (r->tree == NULL) printf("Root is NULL\n");
		if (!IS_ROOT(r->tree)) printf("Root not set as root");
		if (GET_REP(r->tree) != (void*)r) printf("Rep & root are not linked\n");
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
		
		if(IS_ROOT(node)) printf("[0] ");
		else printf("[1] ");
		printf("Addr(%lu)\n", node->addr);
		
		if (node->addr == 0) dummy_nodes++;
		else real_nodes++;
		
		if (node->next == head) {
			printf("Linked correctly to head\n");
			break;
		}
		
		node = node->next;
		
	}
	
	if (real_nodes != reals) printf("Wrong # real nodes\n");
	if (dummy_nodes != dummies) printf("Wrong # dummy nodes\n");
	
}
#endif
