/*
 * Union find allocator
 * 
 * Last changed: $Date: 2011-05-15 20:18:14 +0200 (Sun, 15 May 2011) $
 * Revision:     $Rev: 19 $
 */
 
#include "pool.h"
#include "union_find.h"
#include "uf_alloc.h"

#define PAGE_REPS  1024
#define PAGE_NODES 1024

void * uf_alloc_init(){

	struct pool * p = calloc(1, sizeof(struct pool));
	p->pool_node = pool_init(PAGE_NODES, sizeof(Node), &p->free_list_node);
	p->pool_rep = pool_init(PAGE_REPS, sizeof(Representative), &p->free_list_rep); 
	return p;

}

void * uf_alloc_node(struct pool * p) {
	
	void * addr; 
	pool_alloc(p->pool_node, p->free_list_node, addr, Node);
	return addr;
	
}

void * uf_alloc_rep(struct pool * p) {

	void * addr; 
	pool_alloc(p->pool_rep, p->free_list_rep, addr, Representative);
	return addr;
	
}

void uf_free_node(struct pool * p, void * node) {

	pool_free(node, p->free_list_node);
	return;

}

void uf_free_rep(struct pool * p, void * node) {

	pool_free(node, p->free_list_rep);
	return;

}

void uf_alloc_destroy(struct pool * p) {

	pool_cleanup(p->pool_node);
	pool_cleanup(p->pool_rep);
	free(p);
	return;

}
