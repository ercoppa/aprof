/*
 * CCT functions
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

#if CCT

CCTNode * parent_CCT(ThreadData * tdata) {
	
	#if DEBUG
	if (tdata == NULL) failure("Invalid tdata in parent cct"); 
	if (tdata->stack_depth == 0) failure("Invalid stack_depth");
	#endif
	
	if (tdata->stack_depth <= 1) 
		return tdata->root;
	
	return get_activation(tdata, tdata->stack_depth - 1)->node;

}

// -------------------------------------------------------------
// Deallocate calling context tree
// -------------------------------------------------------------
void freeTree(CCTNode * root) {

	// skip empty subtrees
	if (root == NULL) return;

	// call recursively function on children
	CCTNode * node = root->firstChild;
	while(node != NULL) {
		node = node->nextSibling;
		freeTree(node);
	}

	// deallocate CCT node
	VG_(free)(root);

}

// -------------------------------------------------------------
// CCT info generation routine
// -------------------------------------------------------------
void print_cct_info(FILE * f, CCTNode* root, UWord parent_id) {
	// skip empty subtrees
	if (root == NULL) return;
	if (root->context_id > 0) {
		
		char msg[256];
		if (parent_id > 0)
			VG_(sprintf)(msg, "x %llu %llu %lu\n", root->routine_id, root->context_id, parent_id);
		else
			VG_(sprintf)(msg, "x %llu %llu -1\n", root->routine_id, root->context_id);
		
		ap_fwrite(f, msg, VG_(strlen(msg)));
		
	}

	// call recursively function on children
	CCTNode* theNodePtr;
	for (theNodePtr = root->firstChild;
		theNodePtr != NULL;
		theNodePtr = theNodePtr->nextSibling)
			print_cct_info(f, theNodePtr, root->context_id);
}

/*
void print_CCT(CCTNode* root) {
	if (root == NULL) return;
	
	VG_(printf)("Node(%u): %llu %llu - %u - %u - %u\n", 
		sizeof(CCTNode), root->context_id, root->routine_id, (unsigned int) root,
		(unsigned int) root->firstChild, (unsigned int) root->nextSibling);
	CCTNode * node = root->firstChild;
	while(node != NULL) {
		node = node->nextSibling;
		print_CCT(node);
	}

}
*/

#endif
