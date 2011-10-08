
/*--------------------------------------------------------------------*/
/*--- Modified  version of hash table implementation of Valgrind   ---*/
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

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

/* Generic type for a separately-chained hash table.  Via a kind of dodgy
   C-as-C++ style inheritance, tools can extend the VgHashNode type, so long
   as the first two fields match the sizes of these two fields.  Requires
   a bit of casting by the tool. */

// Problems with this data structure:
// - Separate chaining gives bad cache behaviour.  Hash tables with linear
//   probing give better cache behaviour.

typedef struct HashNode {
        UWord              key;
        struct HashNode  * next;
} HashNode;

struct _HashTable {
   UWord        key;        // Needed because we do ht fo ht...
   void       * next;
   UInt         n_chains;   // should be prime
   UInt         n_elements;
   HashNode  *  iterNode;   // current iterator node
   UInt         iterChain;  // next chain to be traversed by the iterator
   HashNode  ** chains;     // expanding array of hash chains
   Bool         iterOK;     // table safe to iterate over?
   void         (*free_func)(void *); // function invoked on node->value when desctructing the ht
};

typedef struct _HashTable HashTable;

/* Make a new table.  Allocates the memory with VG_(calloc)(), so can
   be freed with VG_(free)().  The table starts small but will
   periodically be expanded.  This is transparent to the users of this
   module. */
extern HashTable * HT_construct (void * free_func);

/* Count the number of nodes in a table. */
extern Int HT_count_nodes (HashTable * table);

/* Add a node to the table.  Duplicate keys are permitted. */
extern void HT_add_node (HashTable * t, UWord key, void * node);

/* Looks up a VgHashNode in the table.  Returns NULL if not found.  If entries
 * with duplicate keys are present, the most recently-added of the dups will
 * be returned, but it's probably better to avoid dups altogether. */
extern void * HT_lookup (HashTable * table, UWord key);

/* Removes a VgHashNode from the table.  Returns NULL if not found. */
extern void * HT_remove (HashTable * table, UWord key);

/* Reset the table's iterator to point to the first element. */
extern void HT_ResetIter (HashTable * table);

/* Return the element pointed to by the iterator and move on to the
   next one.  Returns NULL if the last one has been passed, or if
   HT_ResetIter() has not been called previously.  Asserts if the
   table has been modified (HT_add_node, HT_remove) since
   HT_ResetIter.  This guarantees that callers cannot screw up by
   modifying the table whilst iterating over it (and is necessary to
   make the implementation safe; specifically we must guarantee that
   the table will not get resized whilst iteration is happening.
   Since resizing only happens as a result of calling HT_add_node,
   disallowing HT_add_node during iteration should give the required
   assurance. */
extern void * HT_Next (HashTable * table);

/* Destroy a table. */
extern void HT_destruct (HashTable * t);

#endif 

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
