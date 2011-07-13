/*
 * Header of union find allocator
 * 
 * Last changed: $Date: 2011-06-07 18:44:23 +0200 (Tue, 07 Jun 2011) $
 * Revision:     $Rev: 42 $
 */
 
#ifndef _UNIONFIND_ALLOC_H_
#define _UNIONFIND_ALLOC_H_

struct pool{
	void * pool_node;
	void * free_list_node;
	void * pool_rep;
	void * free_list_rep;
};

// ---------------------------------------------------------------------
// uf_alloc_init
// ---------------------------------------------------------------------
// Setup the allocator, returns the pool's address
//
void * uf_alloc_init(void);

// ---------------------------------------------------------------------
// uf_alloc_node
// ---------------------------------------------------------------------
// Get a new node/representative (not cleared!)
//
void * uf_alloc_node(struct pool * p);
void * uf_alloc_rep(struct pool * p);

// ---------------------------------------------------------------------
// uf_free_node
// ---------------------------------------------------------------------
// Free a node/representative
//
void uf_free_node(struct pool * p, void * node);
void uf_free_rep(struct pool * p, void * node);

// ---------------------------------------------------------------------
// uf_alloc_destroy
// ---------------------------------------------------------------------
// Destroy the allocator (pool) with all allocated objects
//
void uf_alloc_destroy(struct pool * p);

#endif
