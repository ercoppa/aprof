// =====================================================================
//  dc/src/pool.c
// =====================================================================

//  Author:         (C) 2010-2011 Camil Demetrescu
//  License:        See the end of this file for license information
//  Created:        October 13, 2010
//  Module:         dc

//  Last changed:   $Date$
//  Changed by:     $Author: demetres $
//  Revision:       $Revision$


#include "pool.h"

#ifdef GLIBC
#include <stdio.h>
#include <stdlib.h>
#else
#include "glibc-valgrind.h"
#endif

// pool iterator
struct pool_iterator_t {
    pool_t* pool;
    size_t  num_free_blocks;
    size_t  curr_free_block_idx;
    char**  sorted_free_blocks;
    char*   curr_page;
    char*   curr_block;
    char*   curr_page_limit;
};


// private functions
#ifdef GLIBC
static int _pool_addr_compar(const void* x, const void* y);
#else
static int _pool_addr_compar(void* x, void* y);
#endif
static size_t _lookup_addr_le(char** array, size_t size, char* addr);
static void _advance_current_block(pool_iterator_t* it);


// ---------------------------------------------------------------------
// pool_init
// ---------------------------------------------------------------------
// create new allocation pool
//     size_t page_size: number of blocks in each page
//     size_t block_size: number of bytes per block (payload)
//     void** free_list_ref: address of void* object to contain the 
//                           list of free blocks
pool_t* pool_init(size_t page_size, size_t block_size, 
                  void** free_list_ref) {

    pool_t* p = (pool_t*)malloc(sizeof(pool_t));
    if (p == NULL) return NULL;

    p->first_page  = NULL;
    p->page_size   = page_size;
    p->block_size  = block_size;

    *free_list_ref = NULL;

    return p;
}


// ---------------------------------------------------------------------
// pool_cleanup
// ---------------------------------------------------------------------
// dispose of pool
//     pool_t* p: address of pool previously created with pool_init
void pool_cleanup(pool_t* p) {
    void* ptr = p->first_page;
    while (ptr != NULL) {
        void* temp = ptr;
        ptr = *(void**)ptr;
        free(temp);
    }
    free(p);
}


// ---------------------------------------------------------------------
// pool_get_num_blocks
// ---------------------------------------------------------------------
// get number of allocated blocks (both free and in use)
// runs in O(n/page_size), where n is the returned number of blocks
// and page_size is the number of blocks per page (see pool_init)
//     pool_t* p: address of pool previously created with pool_init
size_t pool_get_num_blocks(pool_t* p) {

    size_t num_pages = 0;
    void*  page;

    // count number of pages
    for (page = p->first_page; page != NULL; page = *(void**)page)
        num_pages++;

    return num_pages * p->page_size;
}


// ---------------------------------------------------------------------
// pool_get_num_free_blocks
// ---------------------------------------------------------------------
// get number of free blocks
// runs in O(n) time, where n is the number of free blocks
//     void* free_list: pointer to head of pool's free list
size_t pool_get_num_free_blocks(void* free_list) {

    size_t num_free = 0;
    void*  block;

    // count number of free blocks
    for (block = free_list; block != NULL; block = *(void**)block) 
        num_free++;

    return num_free;
}


// ---------------------------------------------------------------------
// pool_get_num_used_blocks
// ---------------------------------------------------------------------
// get number of blocks in use
// runs in O(f+t/page_size) time, where f is the number of free blocks,
// t is the total number of blocks, and page_size is the number of 
// blocks per page (see pool_init)
//     pool_t* p: address of pool previously created with pool_init
//     void* free_list: pointer to head of pool's free list
size_t pool_get_num_used_blocks(pool_t* p, void* free_list) {
    return pool_get_num_blocks(p) - pool_get_num_free_blocks(free_list);
}


// ---------------------------------------------------------------------
// pool_dump
// ---------------------------------------------------------------------
// print to stdout statistics about pool
//     pool_t* p: address of pool previously created with pool_init
//     void* free_list: address of first free block
//     int dump_heap: if not zero, dumps also pages and blocks
void pool_dump(pool_t* p, void* free_list, int dump_heap) {

    size_t num_pages = 0, num_free = 0, i, j;
    void *page, *block;

    // count number of pages
    for (page = p->first_page; page != NULL; page = *(void**)page)
        num_pages++;

    // count number of free blocks
    for (block = free_list; block != NULL; block = *(void**)block)
        num_free++;


    printf("---- pool %p ----\n", p);
    printf(". number of blocks per page: %u\n", (unsigned int) p->page_size);
    printf(". number of bytes per block: %u\n", p->block_size);
    printf(". number of pages allocated with malloc: %u\n", num_pages);
    printf(". number of blocks: %u\n", num_pages*p->page_size);
    printf(". number allocated blocks: %u\n", 
                                    num_pages*p->page_size - num_free);
    printf(". number free blocks: %u\n", num_free);
    printf(". overall size of pool in bytes: %u "
           "(+%u libc malloc block headers)\n",
        sizeof(pool_t) + num_pages * (sizeof(void*) + 
                         p->page_size * p->block_size),
        num_pages);


    if (!dump_heap) return;

    // scan pages
    for ( page = p->first_page, j = 0; 
          page != NULL; page = *(void**)page, 
          ++j ) {

        void* limit;

        printf(". [page %u @ %p]:\n", j, page);

        // scan blocks
        limit = (char*)page + sizeof(void*) + 
                p->page_size * p->block_size;
        for ( block = (char*)page + sizeof(void*);
              block != limit; 
              block = (char*)block + p->block_size ) {

            printf("    %p : [ ", block);
            for (i = 0; i < p->block_size; i++) {
                unsigned byte = ((unsigned char*)block)[i];
                printf("%s%s%X", i > 0 ? "-" : "", 
                            byte < 0x10 ? "0" : "" , byte);
            }
            printf(" ] %s\n", 
                block == free_list ? "<-- first free block" : "");
        }
    }
}


// ---------------------------------------------------------------------
// pool_iterator_new
// ---------------------------------------------------------------------
// create new pool iterator
// running time: O(f*log(f))
// temporary space required by each iterator: f*sizeof(void*) bytes,
// where f = number of free blocks
//     pool_t* p: address of pool previously created with pool_init
//     void* free_list: address of first free block
pool_iterator_t* pool_iterator_new(pool_t* p, void* free_list) {

    size_t           i;
    void*            block;
    pool_iterator_t* it;

    // allocate iterator
    it = (pool_iterator_t*)malloc(sizeof(pool_iterator_t));
    if (it == NULL) return NULL;

    // if pool is empty then create void iterator
    if (p->first_page == NULL) {
        it->curr_block         = NULL;
        it->sorted_free_blocks = NULL;  
        return it;
    }

    // init fields
    it->curr_page           = (char*)p->first_page;
    it->pool                = p;
    it->num_free_blocks     = 0;
    it->curr_block          = it->curr_page + sizeof(void*);
    it->curr_page_limit     = 
        it->curr_page + sizeof(void*) + p->page_size * p->block_size;

    // count number of free blocks
    for (block = free_list; block != NULL; block = *(void**)block)
        it->num_free_blocks++;

    // allocate array of addresses of free blocks
    it->sorted_free_blocks = 
        (char**)malloc(it->num_free_blocks*sizeof(char*));
    if (it->sorted_free_blocks == NULL) {
        free(it);
        return NULL;
    }

    // populate array of addresses of free blocks
    for (i = 0, block = free_list; 
         block != NULL; 
         block = *(void**)block) it->sorted_free_blocks[i++] = block;

    // sort array of addresses of free blocks in increasing order
    qsort(it->sorted_free_blocks, it->num_free_blocks, 
          sizeof(char*), _pool_addr_compar);

    // find the index of the smallest address of a free block larger 
    // than or equal to the address of the current block;
    // the operation takes O(log(it->num_free_blocks)) time
    it->curr_free_block_idx = 
        _lookup_addr_le(it->sorted_free_blocks, 
                        it->num_free_blocks, 
                        it->curr_block);

    return it;
}


// ---------------------------------------------------------------------
// pool_iterator_next_block
// ---------------------------------------------------------------------
// get pointer to next non-free block, or NULL if there are no more 
// such blocks
void* pool_iterator_next_block(pool_iterator_t* it) {

    char* next_block;

    // pool is empty
    if (it->curr_block == NULL) return NULL;

    // skip free blocks
    while (it->curr_free_block_idx < it->num_free_blocks && 
           it->curr_block == 
               it->sorted_free_blocks[it->curr_free_block_idx]) {
        it->curr_free_block_idx++;
        _advance_current_block(it);
    }

    // select address of block to be returned
    next_block = it->curr_block;

    // advance current block
    _advance_current_block(it);

    return next_block;
}


// ---------------------------------------------------------------------
// pool_iterator_delete
// ---------------------------------------------------------------------
// delete iterator
void  pool_iterator_delete(pool_iterator_t* it) {
    if (it->sorted_free_blocks != NULL) 
        free(it->sorted_free_blocks);
    free(it);
}


// ---------------------------------------------------------------------
// _pool_add_page
// ---------------------------------------------------------------------
// allocate new page and carve new block from it (for internal use...)
void* _pool_add_page(pool_t* p, void** free_list_ref) {

    void *page;
    char *ptr, *limit;
    size_t block_size = p->block_size;

    // allocate page
    page = malloc(sizeof(void*) + p->page_size * block_size);
    if (page == NULL) return NULL;

    // add page to page chain
    *(void**)page = p->first_page;
    p->first_page = page;

    // add all blocks in the page except the first one to the list of 
    // free blocks. Blocks are added to the front of the list in 
    // reverse order. The head of the list of free blocks is made to
    // point to the last block in the new page.
    ptr = (char*)page + sizeof(void*) + block_size;
    *(void**)ptr = *free_list_ref;
    limit = ptr + (p->page_size - 1) * block_size;
    for (ptr += block_size; ptr != limit; ptr += block_size)
        *(void**)ptr = (void*)(ptr - block_size);
    *free_list_ref = (void*)(limit - block_size);

    // allocate first block in page
    return (void*)((void**)page + 1);
}


// ---------------------------------------------------------------------
// _pool_addr_compar
// ---------------------------------------------------------------------
// compare two pointers (for internal use...)
#ifdef GLIBC
static int _pool_addr_compar(const void* x,const void* y) {
#else
static int _pool_addr_compar(void* x,void* y) {
#endif

    char **xp = (char**)x, **yp = (char**)y; 
    return (*xp < *yp) ? -1 : (*xp > *yp) ? +1 : 0;
}



// ---------------------------------------------------------------------
// _lookup_addr_le
// ---------------------------------------------------------------------
// do binary search in array of addresses
// return the index of the smallest address in array larger or equal 
// than addr, or size if all addresses in array are strictly smaller 
// than addr
// precondition: size > 0
static size_t _lookup_addr_le(char** array, size_t size, char* addr) {
    size_t i = 0, j = size;
    while (i < j) {
        size_t m = (i+j)/2;
        if (array[m] == addr) return m;
        if (array[m] <  addr) i = m + 1;
        else                  j = m;
    }
    return i;
}


// ---------------------------------------------------------------------
// _advance_current_block
// ---------------------------------------------------------------------
static void _advance_current_block(pool_iterator_t* it) {

    // if iterator is empty then skip
    if (it->curr_block == NULL) return;

    // advance block
    it->curr_block += it->pool->block_size;

    // if we have reached the end of the page, then move to the next one
    if (it->curr_block == it->curr_page_limit) {

        // let next page be the new current page
        it->curr_page = *(char**)it->curr_page;

        // no more pages left, iterator gets empty
        if (it->curr_page == NULL) {
            it->curr_block = NULL;
            return;
        }

        // continue in next page
        it->curr_block = it->curr_page + sizeof(void*);
        it->curr_page_limit = 
            it->curr_page + sizeof(void*) + 
                it->pool->page_size * it->pool->block_size;

        // find the index of the smallest address of a free block larger 
        // than or equal to the address of the current block;
        // the operation takes O(log(it->num_free_blocks)) time
        it->curr_free_block_idx = 
            _lookup_addr_le(it->sorted_free_blocks, 
                            it->num_free_blocks, 
                            it->curr_block);
    }
}


// Copyright (C) 2010-2011 Camil Demetrescu

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
