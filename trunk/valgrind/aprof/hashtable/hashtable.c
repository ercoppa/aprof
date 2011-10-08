
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

#include "../aprof.h"
#define vg_assert tl_assert

/*--------------------------------------------------------------------*/
/*--- Declarations                                                 ---*/
/*--------------------------------------------------------------------*/

#define CHAIN_NO(key,tbl) (((UWord)(key)) % tbl->n_chains)

#define N_HASH_PRIMES 24

static SizeT primes[N_HASH_PRIMES] = {
          53UL,           97UL,          193UL,           389UL,
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
     
   #if DEBUG_ALLOCATION
   int i = 0;
   for (i = 0; i < n_chains; i++)
      add_alloc(HTNC);
   #endif

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
   HashNode ** chains;
   HashNode  * node;

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

   #if DEBUG_ALLOCATION
   int q = 0;
   for (q = 0; q < (new_chains - table->n_chains); q++)
      add_alloc(HTNC);
   #endif

   table->n_chains = new_chains;
   sz = new_chains * sizeof(HashNode *);
   chains = VG_(calloc)("chains", 1, sz);

   for (i = 0; i < old_chains; i++) {
      node = table->chains[i];
      while (node != NULL) {
         HashNode * next = node->next;
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
void HT_add_node (HashTable * table, UWord key, void * n)
{
   HashNode * node      = (HashNode *) n;
   node->next           = NULL;
   UWord chain          = CHAIN_NO(key, table);
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
void * HT_lookup (HashTable * table, UWord key)
{
	HashNode * curr = table->chains[ CHAIN_NO(key, table) ];

	while (curr) {
		if (key == curr->key)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

/* Removes a VgHashNode from the table.  Returns NULL if not found. */
void * HT_remove (HashTable * table, UWord key)
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
         return curr;
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

void * HT_Next(HashTable * table)
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
      return table->iterNode;
   }

   for (i = table->iterChain; i < table->n_chains; i++) {
      if (table->chains[i]) {
         table->iterNode  = table->chains[i];
         table->iterChain = i + 1;  // Next chain to be traversed
         if (table->iterNode == NULL) return NULL;
         return table->iterNode;
      }
   }
   return NULL;
}

void HT_destruct(HashTable * table)
{
    
   if (table == NULL) return;
   
   UInt       i;
   HashNode *node, *node_next;

   for (i = 0; i < table->n_chains; i++) {
      for (node = table->chains[i]; node != NULL; node = node_next) {
         node_next = node->next;
         if (table->free_func != NULL)
             table->free_func(node);
      }
   }
   //VG_(printf)("I will free %p for %p\n", table->chains, table);
   VG_(free)(table->chains);
   VG_(free)(table);
}


/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
