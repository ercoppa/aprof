// =====================================================================
//  dc/include/pool.h
// =====================================================================

//  Author:         (C) 2010-2011 Camil Demetrescu
//  License:        See the end of this file for license information
//  Created:        October 13, 2010
//  Module:         dc

//  Last changed:   $Date$
//  Changed by:     $Author: demetres $
//  Revision:       $Revision$


#ifndef __pool__
#define __pool__

#ifndef GLIBC
#include "pub_tool_basics.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#define size_t       unsigned int

#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


// pool object structure
typedef struct {
    void**   first_page;    // pointer to first page
    size_t   page_size;     // page size as number of blocks
    size_t   block_size;    // block size in bytes
} pool_t;

// pool iterator
typedef struct pool_iterator_t pool_iterator_t;


// ---------------------------------------------------------------------
// pool_init
// ---------------------------------------------------------------------
// create new allocation pool
//     size_t page_size: number (>= 2) of blocks in each page
//     size_t block_size: number (>= 4) of bytes per block (payload)
//     void** free_list_ref: address of void* object to contain the 
//                           list of free blocks
pool_t* pool_init(size_t page_size, size_t block_size, 
                  void** free_list_ref);


// ---------------------------------------------------------------------
// pool_cleanup
// ---------------------------------------------------------------------
// dispose of pool
//     pool_t* p: address of pool previously created with pool_init
void pool_cleanup(pool_t* p);


// ---------------------------------------------------------------------
// pool_get_num_blocks
// ---------------------------------------------------------------------
// get number of allocated blocks (both free and in use)
// runs in O(n/page_size), where n is the returned number of blocks
// and page_size is the number of blocks per page (see pool_init)
//     pool_t* p: address of pool previously created with pool_init
size_t pool_get_num_blocks(pool_t* p);


// ---------------------------------------------------------------------
// pool_get_num_free_blocks
// ---------------------------------------------------------------------
// get number of free blocks
// runs in O(n) time, where n is the number of free blocks
//     void* free_list: pointer to head of pool's free list
size_t pool_get_num_free_blocks(void* free_list);


// ---------------------------------------------------------------------
// pool_get_num_used_blocks
// ---------------------------------------------------------------------
// get number of blocks in use
// runs in O(f+t/page_size) time, where f is the number of free blocks,
// t is the total number of blocks, and page_size is the number of 
// blocks per page (see pool_init)
//     pool_t* p: address of pool previously created with pool_init
//     void* free_list: pointer to head of pool's free list
size_t pool_get_num_used_blocks(pool_t* p, void* free_list);


// ---------------------------------------------------------------------
// pool_iterator_new
// ---------------------------------------------------------------------
// create new pool iterator
// running time: O(f*log(f))
// temporary space required by each iterator: f*sizeof(void*) bytes,
// where f = number of free blocks
//     pool_t* p: address of pool previously created with pool_init
pool_iterator_t* pool_iterator_new(pool_t* p, void* free_list);


// ---------------------------------------------------------------------
// pool_iterator_next_block
// ---------------------------------------------------------------------
// get pointer to next used block, or NULL if there are no more blocks
void* pool_iterator_next_block(pool_iterator_t* iterator);


// ---------------------------------------------------------------------
// pool_iterator_delete
// ---------------------------------------------------------------------
// delete iterator
void  pool_iterator_delete(pool_iterator_t* iterator);


// ---------------------------------------------------------------------
// pool_dump
// ---------------------------------------------------------------------
// print to stdout statistics about pool
//     pool_t* p: address of pool previously created with pool_init
//     void* free_list: address of first free block
//     int dump_heap: if not zero, dumps also pages and blocks
void pool_dump(pool_t* p, void* free_list, int dump_heap);


// ---------------------------------------------------------------------
// _pool_add_page
// ---------------------------------------------------------------------
// for internal use...
void* _pool_add_page(pool_t* p, void** free_list);


// ---------------------------------------------------------------------
// pool_alloc (macro)
// ---------------------------------------------------------------------
// allocate new block 
// note: macro expansion yields a pointer expression
//     pool_t* pool: address of pool object created with pool_init
//     void* free_list: lvalue containing the address of first block in 
//                      a list of free blocks
//     void* out: lvalue to be assigned with the address of the newly 
//                allocated block
#define pool_alloc(pool, free_list, out, block_type) \
    (free_list ? ((out) = (block_type*)(free_list), (free_list) =      \
                                               *(void**)(free_list)) : \
                 ((out) = (block_type*)_pool_add_page((pool),          \
                                                        &free_list)))


// ---------------------------------------------------------------------
// pool_free (macro)
// ---------------------------------------------------------------------
// free block
// note: macro expansion yields a compound statement
//     void* block_ptr: address of block to be freed
//     void* free_list: lvalue containing the address of the first block
//                      in a list of free blocks 
#define pool_free(block_ptr, free_list) do {                          \
    void* _free_tmp = (void*)(block_ptr);                             \
    (*(void**)_free_tmp = (free_list), (free_list) = _free_tmp);      \
} while(0)

#ifdef __cplusplus
}
#endif

#endif


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
