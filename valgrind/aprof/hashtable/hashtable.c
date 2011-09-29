
/*--------------------------------------------------------------------*/
/*--- A separately-chained hash table.               m_hashtable.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Valgrind, a dynamic binary instrumentation
   framework.

   Copyright (C) 2005-2010 Nicholas Nethercote
      njn@valgrind.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "hashtable.h"
#include "../pool/pool.h"
#define vg_assert tl_assert

/*--------------------------------------------------------------------*/
/*--- Declarations                                                 ---*/
/*--------------------------------------------------------------------*/

/* One pool for all hashtables */
static pool_t * pool = NULL;
static void * free_list = NULL;
#define PAGE_NODES 1024

#define CHAIN_NO(key,tbl) (((UWord)(key)) % tbl->n_chains)

struct _HashTable {
   UInt         n_chains;   // should be prime
   UInt         n_elements;
   HashNode  *  iterNode;   // current iterator node
   UInt         iterChain;  // next chain to be traversed by the iterator
   HashNode  ** chains;     // expanding array of hash chains
   Bool         iterOK;     // table safe to iterate over?
   void         (*free_func)(void *); // function invoked on node->value when desctructing the ht
};

#define N_HASH_PRIMES 20

static SizeT primes[N_HASH_PRIMES] = {
         769UL,         1543UL,         3079UL,          6151UL,
       12289UL,        24593UL,        49157UL,         98317UL,
      196613UL,       393241UL,       786433UL,       1572869UL,
     3145739UL,      6291469UL,     12582917UL,      25165843UL,
    50331653UL,    100663319UL,    201326611UL,     402653189UL
};

/*--------------------------------------------------------------------*/
/*--- Functions                                                    ---*/
/*--------------------------------------------------------------------*/

HashTable * HT_construct(void * func)
{
   /* Initialises to zero, ie. all entries NULL */
   SizeT       n_chains = primes[0];
   SizeT       sz       = n_chains * sizeof(HashNode *);
   HashTable * table    = VG_(calloc)("ht", 1, sizeof(struct _HashTable));
   table->chains        = VG_(calloc)("chains", 1, sz);
   table->n_chains      = n_chains;
   table->n_elements    = 0;
   table->iterOK        = True;
   table->free_func     = func;
   
   if (pool == NULL)
		pool = pool_init(PAGE_NODES, sizeof(HashNode), &free_list);
   
   return table;
}

Int HT_count_nodes (HashTable * table)
{
   return table->n_elements;
}

static void resize (HashTable * table)
{
   Int          i;
   SizeT        sz;
   SizeT        old_chains = table->n_chains;
   SizeT        new_chains = old_chains + 1;
   HashNode** chains;
   HashNode * node;

   /* If we've run out of primes, do nothing. */
   if (old_chains == primes[N_HASH_PRIMES-1])
      return;

   vg_assert(old_chains >= primes[0] 
             && old_chains < primes[N_HASH_PRIMES-1]);

   for (i = 0; i < N_HASH_PRIMES; i++) {
      if (primes[i] > new_chains) {
         new_chains = primes[i];
         break;
      }
   }

   vg_assert(new_chains > old_chains);
   vg_assert(new_chains > primes[0] 
             && new_chains <= primes[N_HASH_PRIMES-1]);

   table->n_chains = new_chains;
   sz = new_chains * sizeof(HashNode*);
   chains = VG_(calloc)("chains", 1, sz);

   for (i = 0; i < old_chains; i++) {
      node = table->chains[i];
      while (node != NULL) {
         HashNode* next = node->next;
         UWord chain = CHAIN_NO(node->key, table);
         node->next = chains[chain];
         chains[chain] = node;
         node = next;
      }
   }

   VG_(free)(table->chains);
   table->chains = chains;
}

/* Puts a new, heap allocated VgHashNode, into the VgHashTable.  Prepends
   the node to the appropriate chain.  No duplicate key detection is done. */
void HT_add_node (HashTable * table, UWord key, void * value)
{
   HashNode * node = NULL;
   pool_alloc(pool, free_list, node, HashNode);
   node->next           = NULL;
   node->key            = key;
   node->value          = value;
   UWord chain          = CHAIN_NO(node->key, table);
   node->next           = table->chains[chain];
   table->chains[chain] = node;
   table->n_elements++;
   if ( (1 * (ULong)table->n_elements) > (1 * (ULong)table->n_chains) ) {
      resize(table);
   }

   /* Table has been modified; hence HT_Next should assert. */
   table->iterOK = False;
}

/* Looks up a VgHashNode in the table.  Returns NULL if not found. */
void * HT_lookup (HashTable * table, UWord key, HashNode ** node)
{
	HashNode * curr = table->chains[ CHAIN_NO(key, table) ];

	while (curr) {
		if (key == curr->key) {
			if (node != NULL) *node = curr;
			return curr->value;
		}
		curr = curr->next;
	}
	return NULL;
}

/* Removes a VgHashNode from the table.  Returns NULL if not found. */
void* HT_remove (HashTable * table, UWord key)
{
   UWord      chain         = CHAIN_NO(key, table);
   HashNode*  curr          = table->chains[chain];
   HashNode** prev_next_ptr = &(table->chains[chain]);

   /* Table has been modified; hence HT_Next should assert. */
   table->iterOK = False;

   while (curr) {
      if (key == curr->key) {
         *prev_next_ptr = curr->next;
         table->n_elements--;
         void * value = curr->value;
         pool_free(curr, free_list);
         return value;
      }
      prev_next_ptr = &(curr->next);
      curr = curr->next;
   }
   return NULL;
}

void HT_ResetIter(HashTable * table)
{
   vg_assert(table);
   table->iterNode  = NULL;
   table->iterChain = 0;
   table->iterOK    = True;
}

void * HT_Next(HashTable * table, UWord * key, void ** value)
{
   Int i;
   vg_assert(table);
   /* See long comment on HT_Next prototype in pub_tool_hashtable.h.
      In short if this fails, it means the caller tried to modify the
      table whilst iterating over it, which is a bug. */
   vg_assert(table->iterOK);

   if (table->iterNode && table->iterNode->next) {
      table->iterNode = table->iterNode->next;
      if (table->iterNode == NULL) return NULL;
      *key = table->iterNode->key;
      *value = table->iterNode->value;
      return table->iterNode;
   }

   for (i = table->iterChain; i < table->n_chains; i++) {
      if (table->chains[i]) {
         table->iterNode  = table->chains[i];
         table->iterChain = i + 1;  // Next chain to be traversed
         if (table->iterNode == NULL) return NULL;
         *key = table->iterNode->key;
         *value = table->iterNode->value;
         return table->iterNode;
      }
   }
   return NULL;
}

void HT_destruct(HashTable * table)
{
   UInt       i;
   HashNode *node, *node_next;

   for (i = 0; i < table->n_chains; i++) {
      for (node = table->chains[i]; node != NULL; node = node_next) {
         node_next = node->next;
         if (table->free_func != NULL)
             table->free_func(node->value);
         pool_free(node, free_list);
      }
   }
   VG_(free)(table->chains);
   VG_(free)(table);
}

void  HT_destroy_pool(void) {
	pool_cleanup(pool);
}

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
