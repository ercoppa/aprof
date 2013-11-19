/*
 * CCT functions
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2014, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi,
                            Romolo Marotta

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

#include "aprof.h"

// Return the CCT node related to the deepest active activation
CCTNode * APROF_(parent_CCT)(ThreadData * tdata) {

    APROF_(debug_assert)(tdata != NULL, "Invalid tdata");
    APROF_(debug_assert)(tdata->stack_depth > 0, "Invalid stack_depth");
    
    if (tdata->stack_depth <= 1)
        return tdata->root;

    UInt depth = tdata->stack_depth;
    CCTNode * node = APROF_(get_activation)(tdata, depth - 1)->node;
    
    // some activations do not have a CCT node, skip them
    while (node == NULL) {
        depth--;
        if (depth <= 1) return tdata->root;
        node = APROF_(get_activation)(tdata, depth - 1)->node;
    }
    return node;
}

// Delete a CCT 
void APROF_(free_CCT)(CCTNode * root) {

    // skip empty subtrees
    if (root == NULL) return;

    // call recursively function on children
    CCTNode * node = root->first_child;
    CCTNode * node2 = NULL;
    while(node != NULL) {
        node2 = node;
        node = node->next_sibling;
        APROF_(free_CCT)(node2);
    }

    // deallocate CCT node
    APROF_(delete)(CCT_S, root);
}

// Output a CCT in a report file
void APROF_(print_report_CCT)(FILE * f, CCTNode * root, UInt parent_id) {

    // skip empty subtrees
    if (root == NULL) return;
    if (root->context_id > 0) {

        if (parent_id > 0)
            APROF_(fprintf)(f, "x %llu %u %u\n", root->routine_id, 
                                root->context_id, parent_id);
        else
            APROF_(fprintf)(f, "x %llu %u -1\n", root->routine_id, 
                                root->context_id);
    }

    // call recursively function on children
    CCTNode * node;
    for (node = root->first_child; node != NULL; node = node->next_sibling)
        APROF_(print_report_CCT)(f, node, root->context_id);
}
