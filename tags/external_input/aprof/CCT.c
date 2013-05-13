/*
 * CCT functions
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi

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

#if CCT

/*
 * Return the root of the CCT
 */
CCTNode * APROF_(parent_CCT)(ThreadData * tdata) {
	
	#if DEBUG
	AP_ASSERT(tdata != NULL, "Invalid tdata");
	AP_ASSERT(tdata->stack_depth > 0, "Invalid stack_depth");
	#endif
	
	if (tdata->stack_depth <= 1) 
		return tdata->root;
	
	int depth = tdata->stack_depth;
	CCTNode * node = APROF_(get_activation)(tdata, depth - 1)->node;
	while (node == NULL) {
		depth--;
		if (depth == 0) return tdata->root;
		node = APROF_(get_activation)(tdata, depth - 1)->node;
	}
	return node;

}

/*
 * Delete a CCT 
 */
void APROF_(free_CCT)(CCTNode * root) {

	//APROF_(print_CCT)(root);
	//return;

	// skip empty subtrees
	if (root == NULL) return;

	// call recursively function on children
	CCTNode * node = root->firstChild;
	CCTNode * node2 = NULL;
	while(node != NULL) {
		node2 = node;
		node = node->nextSibling;
		APROF_(free_CCT)(node2);
	}

	// deallocate CCT node
	VG_(free)(root);

}

/*
 * Output a CCT in a report file
 */
void APROF_(print_report_CCT)(FILE * f, CCTNode* root, UInt parent_id) {
	
	// skip empty subtrees
	if (root == NULL) return;
	if (root->context_id > 0) {
		
		char msg[256];
		if (parent_id > 0)
			VG_(sprintf)(msg, "x %llu %u %u\n", root->routine_id, root->context_id, parent_id);
		else
			VG_(sprintf)(msg, "x %llu %u -1\n", root->routine_id, root->context_id);
		
		APROF_(fwrite)(f, msg, VG_(strlen(msg)));
		
	}

	// call recursively function on children
	CCTNode* theNodePtr;
	for (theNodePtr = root->firstChild;
		theNodePtr != NULL;
		theNodePtr = theNodePtr->nextSibling)
			APROF_(print_report_CCT)(f, theNodePtr, root->context_id);
}


/*
 * Print a CCT
 */
/*
static void APROF_(print_CCT)(CCTNode* root) {
	if (root == NULL) return;
	
	VG_(printf)("%u\n", root->context_id);
	CCTNode * node = root->firstChild;
	while(node != NULL) {
		VG_(printf)("%u ", node->context_id);
		node = node->nextSibling;
	}
	
	VG_(printf)("\n\n");
	
	node = root->firstChild;
	while(node != NULL) {
		print_CCT(node);
		node = node->nextSibling;
	}

}
*/

#endif
