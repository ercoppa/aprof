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
	AP_ASSERT(tdata != NULL, "Invalid tdata");
	AP_ASSERT(tdata->stack_depth > 0, "Invalid stack_depth");
	#endif
	
	if (tdata->stack_depth <= 1) 
		return tdata->root;
	
	return get_activation(tdata, tdata->stack_depth - 1)->node;

}

// -------------------------------------------------------------
// Deallocate calling context tree
// -------------------------------------------------------------
void freeTree(CCTNode * root) {

	//print_CCT(root);
	//return;

	// skip empty subtrees
	if (root == NULL) return;

	// call recursively function on children
	CCTNode * node = root->firstChild;
	CCTNode * node2 = NULL;
	while(node != NULL) {
		node2 = node;
		node = node->nextSibling;
		freeTree(node2);
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

#if CCT_GRAPHIC
void print_cct_graph(FILE * f, CCTNode* root, UWord parent_id, char * parent_name) {
	// skip empty subtrees
	if (root == NULL) return;
	
	char msg[256];
	
	if (VG_(strncmp)(root->name, "check_match", 10) == 0) {
		root->name[11] = '0';
	}
	
	if (parent_name != NULL){

		VG_(sprintf)(msg, "%s->%s%llu;\n", parent_name, root->name, root->context_id);
		ap_fwrite(f, msg, VG_(strlen(msg)));

	}
	VG_(sprintf)(msg, "%s%llu", root->name, root->context_id);

	// call recursively function on children
	CCTNode* theNodePtr;
	for (theNodePtr = root->firstChild;
		theNodePtr != NULL;
		theNodePtr = theNodePtr->nextSibling)
			print_cct_graph(f, theNodePtr, root->context_id, msg);
}
#endif

static void print_CCT(CCTNode* root) {
	if (root == NULL) return;
	
	VG_(printf)("%llu\n", root->context_id);
	CCTNode * node = root->firstChild;
	while(node != NULL) {
		VG_(printf)("%llu ", node->context_id);
		node = node->nextSibling;
	}
	
	VG_(printf)("\n\n");
	
	node = root->firstChild;
	while(node != NULL) {
		print_CCT(node);
		node = node->nextSibling;
	}

}


#endif
