/* ============================================================================
 *  LGraph.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 12, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:52 $  
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "_LGraph.h"
#include "LException.h"
#include "LMemory.h"

/* Definition of private methods */
void _DeAllocNodeList(LGraph_TNode* inPtr);
void _DeAllocEdgeList(LGraph_TEdge* inPtr);


/* PUBLIC METHODS */


/* ---------------------------------------------------------------------------------
*  LGraph_New
*  ---------------------------------------------------------------------------------
*  Constructor */
LGraph*	LGraph_New(Bool inDirected)
{
	LGraph theObject = {0};
	LGraph* theGraph = NULL;

	if (inDirected) 
		theObject.mDirected = TRUE;
	else
		theObject.mDirected = FALSE;

	Try
	{
		theObject.mNodeListArray    = LArray_New( sizeof(LNodeInfo*) );
		theObject.mEdgeListArray    = LArray_New( sizeof(LEdgeInfo*) );
		theObject.mEdgeMapListArray = LArray_New( sizeof(LEdgeMap*) );
		theObject.mDebug            = FALSE;
		theGraph = LMemory_NewObject(LGraph, theObject);
		LArray_InstallSyncPtr(theGraph->mNodeListArray, (void**)&(theGraph->mNodeList));
		LArray_InstallSyncPtr(theGraph->mEdgeListArray, (void**)&(theGraph->mEdgeList));
		LArray_InstallSyncPtr(theGraph->mEdgeMapListArray, 
						      (void**)&(theGraph->mEdgeMapList));
	}
	CatchAny
	{/* if something goes wrong, cleanup */
		if (theObject.mNodeListArray != NULL) LArray_Delete(&theObject.mNodeListArray);
		if (theObject.mEdgeListArray != NULL) LArray_Delete(&theObject.mEdgeListArray);
		if (theObject.mEdgeMapListArray != NULL) 
			LArray_Delete(&theObject.mEdgeMapListArray);
		if (theGraph != NULL) LMemory_DeleteObject(&theGraph);
		Rethrow;
	}
	return theGraph;
}


/* ---------------------------------------------------------------------------------
*  LGraph_Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void LGraph_Delete(LGraph** ThisA)
{
	ui4 i;
    ui4 theCount;
        
    if (ThisA == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if ((*ThisA) == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	
	/* we must set to NULL the pointer in any registered LNodeInfo, LEdgeInfo, LEdgeMap */
    Try
	{
		theCount = LArray_GetItemsCount((*ThisA)->mNodeListArray);
		for (i=0; i < theCount; i++)
			if ((*ThisA)->mNodeList[i])
				_LNodeInfo_CallItemsDestructor((*ThisA)->mNodeList[i]);
		
		theCount = LArray_GetItemsCount((*ThisA)->mEdgeListArray);
		for (i=0; i < theCount; i++)
			if ((*ThisA)->mEdgeList[i])
				_LEdgeInfo_CallItemsDestructor((*ThisA)->mEdgeList[i]);

		theCount = LArray_GetItemsCount((*ThisA)->mEdgeMapListArray);
		for (i=0; i < theCount; i++)
			if ((*ThisA)->mEdgeMapList[i])
				_LEdgeMap_UnregisterGraph((*ThisA)->mEdgeMapList[i]);
	
		/* now, we deallocate the edges/nodes list */
		if ( (*ThisA)->mFirstNode != NULL ) 
			_DeAllocNodeList((*ThisA)->mFirstNode);
		if ( (*ThisA)->mFirstEdge != NULL ) 
			_DeAllocEdgeList((*ThisA)->mFirstEdge);
		
		LArray_Delete( &((*ThisA)->mNodeListArray) ) ;
		LArray_Delete( &((*ThisA)->mEdgeListArray) ) ;
		LArray_Delete( &((*ThisA)->mEdgeMapListArray) );
	}
	CatchAny
		Rethrow;
	LMemory_DeleteObject(ThisA);
}


/* ---------------------------------------------------------------------------------
*  LGraph_NewNode
*  ---------------------------------------------------------------------------------
*  Adds a node and returns a pointer to it */
LGraph_TNode* LGraph_NewNode(LGraph* This)
{
	ui4 theCount;
	ui4 i;

	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);

	if (This->mNodesCount == 0)
	{/* first node */
		This->mFirstNode = (LGraph_TNode *)LMemory_Calloc( sizeof(LGraph_TNode) );
		This->mLastNode = This->mFirstNode;
	}
	else
	{/* inserts at the bottom of the list */
		This->mLastNode->mNext = (LGraph_TNode *)LMemory_Calloc( sizeof(LGraph_TNode) );
		This->mLastNode->mNext->mPrev = This->mLastNode;
		This->mLastNode = This->mLastNode->mNext;
	}
	This->mLastNode->mIndex = This->mNodesCount;
	This->mNodesCount++;
	/* adds a new blank node to each registered LNodeInfo */
	Try
		theCount = LArray_GetItemsCount(This->mNodeListArray);
	CatchAny
		Rethrow;
	for (i=0; i < theCount; i++)
		if (This->mNodeList[i])
			_LNodeInfo_AddInfo(This->mNodeList[i], This->mLastNode);
	
	return This->mLastNode;
}


/* ---------------------------------------------------------------------------------
*  LGraph_DelNode
*  ---------------------------------------------------------------------------------
*  Delete a node of the graph */
void LGraph_DelNode(LGraph* This, LGraph_TNode* inNode)
{
	ui4 theCount;
	ui4 i;
	LGraph_TEdge* theEdge;
	LGraph_TEdge* theTempEdge;
	LGraph_TNode* theSwapNode;
	LException* theException;

	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inNode->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);
	
	if (This->mNodesCount == 1)
	{
		This->mFirstNode = This->mLastNode = NULL;
		if (This->mFirstEdge != NULL ) 
		_DeAllocEdgeList(This->mFirstEdge);
		This->mNodesCount--;
		LMemory_Free(&inNode);
		return;
	}

	if (inNode == This->mFirstNode)
	{/* deletion at the top */
		theSwapNode = This->mLastNode;
		This->mLastNode = This->mLastNode->mPrev;
		This->mFirstNode = theSwapNode;
		theSwapNode->mPrev = NULL;
		theSwapNode->mNext = inNode->mNext;
		inNode->mNext->mPrev = theSwapNode;
		This->mLastNode->mNext = NULL;
	}
	else
	if (inNode == This->mLastNode)
	{/* deletion at the bottom */
		theSwapNode = This->mLastNode;
		This->mLastNode = This->mLastNode->mPrev;
		This->mLastNode->mNext = NULL;
	}
	else
	{/* deletion in the middle */
		theSwapNode = This->mLastNode;
		This->mLastNode = This->mLastNode->mPrev;
		inNode->mNext->mPrev = theSwapNode;
		inNode->mPrev->mNext = theSwapNode;
		theSwapNode->mPrev = inNode->mPrev;
		theSwapNode->mNext = inNode->mNext;
		This->mLastNode->mNext = NULL;
	}
	if (theSwapNode->mNext == theSwapNode)
	{/* deletion in the one before last node */
		This->mLastNode = theSwapNode;
	}

	/* ensures that the lists are correctly closed */
	This->mFirstNode->mPrev = NULL;
	This->mLastNode->mNext = NULL;

	theSwapNode->mIndex = inNode->mIndex;
	
	/* now, we must remove all the in/out Edges of the node */
	theEdge = inNode->mFirstIn;
	while( theEdge!=NULL )
	{ 
		theTempEdge = theEdge;
		theEdge = theEdge->mNextTarget;
		LGraph_DelEdge(This, theTempEdge);
	}

	theEdge = inNode->mFirstOut;
	if (This->mDirected)
	{/* **********Directed Graph Block ************** */
		while( theEdge!=NULL )
		{
			theTempEdge = theEdge;
			theEdge = theEdge->mNextSource;
			LGraph_DelEdge(This, theTempEdge);
		}
	}
	else
	{/* ********Undirected Graph Block ************** */
		while( theEdge!=NULL )
		{
			theTempEdge = theEdge;
			if (theEdge->mSource == inNode)
				theEdge = theEdge->mNextSource;
			else
				theEdge = theEdge->mNextTarget;
			LGraph_DelEdge(This, theTempEdge);
		}
	}
		
	This->mNodesCount--;

	/* now we must remove the node info in all registered LNodeInfo */
	Try
		theCount = LArray_GetItemsCount(This->mNodeListArray);
	Catch(theException)
		LException_Dump(theException);
	for (i=0; i < theCount; i++)
		if (This->mNodeList[i])
			_LNodeInfo_DeleteItemAt(This->mNodeList[i], inNode);
	LMemory_Free(&inNode);
 }


/* ---------------------------------------------------------------------------------
*  LGraph_NewEdge
*  ---------------------------------------------------------------------------------
*  Adds an edge and returns a pointer to it */
LGraph_TEdge* LGraph_NewEdge(LGraph* This, LGraph_TNode* inSource, LGraph_TNode* inTarget)
{
	LGraph_TEdge* theEdge;
	ui4 theCount;
	ui4 i;
	
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inSource == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inTarget == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inSource->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);
	if (inTarget->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);

	/* allocates the edge and update the mSource and mTarget field */
	theEdge = (LGraph_TEdge *)LMemory_Calloc( sizeof(LGraph_TEdge) );
	theEdge->mSource = inSource;
	theEdge->mTarget = inTarget;

	/* links the edge to the list of edges of the graph */
	if (This->mEdgesCount == 0)
	{/* first edge */
		This->mFirstEdge = theEdge;
		This->mLastEdge  = theEdge;
	}
	else
	{/* inserts at the bottom of the list */
		This->mLastEdge->mNext = theEdge;
		theEdge->mPrev = This->mLastEdge;
		This->mLastEdge = theEdge;
	}
	
	if (This->mDirected)
	{ /* **********Directed Graph Block ***************/
		/* links the edge to the list of inEdges of the graph target node*/
		if (theEdge->mTarget->mFirstIn == NULL)
		{/* first inEdge */
			theEdge->mTarget->mFirstIn = theEdge;
			theEdge->mTarget->mLastIn  = theEdge;
		}
		else
		{/* inserts at the bottom of the list */ 
			theEdge->mPrevTarget = theEdge->mTarget->mLastIn;
			theEdge->mTarget->mLastIn->mNextTarget = theEdge;
			theEdge->mTarget->mLastIn = theEdge;
		}

		/* links the edge to the list of outEdges of the graph source node*/
		if (theEdge->mSource->mFirstOut == NULL)
		{/* first inEdge */
			theEdge->mSource->mFirstOut = theEdge;
			theEdge->mSource->mLastOut  = theEdge;
		}
		else
		{/* inserts at the bottom of the list */ 
			theEdge->mPrevSource = theEdge->mSource->mLastOut;
			theEdge->mSource->mLastOut->mNextSource = theEdge;
			theEdge->mSource->mLastOut = theEdge;
		}
		theEdge->mSource->mOutDeg++;
		theEdge->mTarget->mInDeg++;
		
	}
	else
	{ /* ********Undirected Graph Block ***************/

		if ((theEdge->mSource) != (theEdge->mTarget))
		{/* links the edge to the list of outEdges of the graph target node*/
			if (theEdge->mTarget->mFirstOut == NULL)
			{/* first inEdge */
				theEdge->mTarget->mFirstOut = theEdge;
				theEdge->mTarget->mLastOut  = theEdge;
			}
			else
			{/* inserts at the bottom of the list */ 
				theEdge->mPrevTarget = theEdge->mTarget->mLastOut;
				if (inTarget == theEdge->mTarget->mLastOut->mTarget)
					theEdge->mTarget->mLastOut->mNextTarget = theEdge;
				else
					theEdge->mTarget->mLastOut->mNextSource = theEdge;
				theEdge->mTarget->mLastOut = theEdge;
			}
			theEdge->mTarget->mOutDeg++;
		}

		/* links the edge to the list of outEdges of the graph source node*/
		if (theEdge->mSource->mFirstOut == NULL)
		{/* first inEdge */
			theEdge->mSource->mFirstOut = theEdge;
			theEdge->mSource->mLastOut  = theEdge;
		}
		else
		{/* inserts at the bottom of the list */ 
			theEdge->mPrevSource = theEdge->mSource->mLastOut;
			if (inSource == theEdge->mSource->mLastOut->mSource)
				theEdge->mSource->mLastOut->mNextSource = theEdge;
			else
				theEdge->mSource->mLastOut->mNextTarget = theEdge;
			theEdge->mSource->mLastOut = theEdge;
		}
		theEdge->mSource->mOutDeg++;
	}
	theEdge->mIndex = This->mEdgesCount;
	This->mEdgesCount++;
	
	/* adds a new blank edge to each registered LEdgeInfo */
	Try
		theCount = LArray_GetItemsCount(This->mEdgeListArray);
	CatchAny
		Rethrow;
	for (i=0; i < theCount; i++)
		if (This->mEdgeList[i])
			_LEdgeInfo_AddInfo(This->mEdgeList[i], theEdge);

	/* adds a new edge to each registered LEdgeMap */
	Try
		theCount = LArray_GetItemsCount(This->mEdgeMapListArray);
	CatchAny
		Rethrow;
	for (i=0; i < theCount; i++)
		if (This->mEdgeMapList[i])
			_LEdgeMap_AddEdge(This->mEdgeMapList[i], theEdge);
	
	return theEdge;
}


/* ---------------------------------------------------------------------------------
*  LGraph_DelEdge
*  ---------------------------------------------------------------------------------
*  Delete an edge of the graph */
void LGraph_DelEdge(LGraph* This, LGraph_TEdge* inEdge)
{
	LGraph_TEdge* theSwapEdge;
	ui4 theCount;
	ui4 i;

	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inEdge == NULL) Throw(LGraph_EDGE_NULL_POINTER);
	if (inEdge->mIndex >= This->mEdgesCount) Throw(LGraph_EDGE_NOT_IN_GRAPH);
	
	/* deletion from the main list of edges */
	if (This->mEdgesCount == 1)
		This->mFirstEdge = This->mLastEdge = NULL;
	else
	{
		if (inEdge == This->mFirstEdge)
		{/* deletion at the top */
			theSwapEdge = This->mLastEdge;
			This->mLastEdge = This->mLastEdge->mPrev;
			This->mFirstEdge = theSwapEdge;
			theSwapEdge->mPrev = NULL;
			theSwapEdge->mNext = inEdge->mNext;
			inEdge->mNext->mPrev = theSwapEdge;
			This->mLastEdge->mNext = NULL;
		}
		else
		if (inEdge == This->mLastEdge)
		{/* deletion at the bottom */
			theSwapEdge = This->mLastEdge;
			This->mLastEdge = This->mLastEdge->mPrev;
			This->mLastEdge->mNext = NULL;
		}
		else
		{/* deletion in the middle */
			theSwapEdge = This->mLastEdge;
			This->mLastEdge = This->mLastEdge->mPrev;
			inEdge->mNext->mPrev = theSwapEdge;
			inEdge->mPrev->mNext = theSwapEdge;
			theSwapEdge->mPrev = inEdge->mPrev;
			theSwapEdge->mNext = inEdge->mNext;
			This->mLastEdge->mNext = NULL;
		}

		if (theSwapEdge->mNext == theSwapEdge)
		{/* deletion in the one before last node */
			This->mLastEdge = theSwapEdge;
		}

		/* ensures that the lists are correctly closed */
		This->mFirstEdge->mPrev = NULL;
		This->mLastEdge->mNext  = NULL;
		/* fixes the edge index */
		theSwapEdge->mIndex = inEdge->mIndex;
	}

	/* deletion from the edges list of the nodes */
	if (This->mDirected)
	{ /* **********Directed Graph Block ***************/
	    /* deletes the edge from the list of inEdges of the graph target node*/
		if (inEdge->mTarget->mFirstIn == inEdge)
		{/* deletion at the top of the list */
			if (inEdge->mNextTarget != NULL) /* there's at least another edge */
				inEdge->mNextTarget->mPrevTarget = NULL;
			else /* there's only this edge in the list, assign NULL to last element's ptr */
				inEdge->mTarget->mLastIn = NULL;
			inEdge->mTarget->mFirstIn = inEdge->mNextTarget;
        }
		else
		{
			if (inEdge->mTarget->mLastIn == inEdge)
			{/* deletion at the bottom of the list */
				inEdge->mPrevTarget->mNextTarget = NULL;
				inEdge->mTarget->mLastIn = inEdge->mPrevTarget;
			}
			else
			{/* deletion in the middle of the list */
				inEdge->mNextTarget->mPrevTarget = inEdge->mPrevTarget;
				inEdge->mPrevTarget->mNextTarget = inEdge->mNextTarget;
			}
		}

		/* deletes the edge from the list of outEdges of the graph source node*/
		if (inEdge->mSource->mFirstOut == inEdge)
		{/* deletion at the top of the list */
			if(inEdge->mNextSource != NULL) /* there's at least another edge */
				inEdge->mNextSource->mPrevSource = NULL;
			else /* there's only this edge in the list, assign NULL to last element's ptr */
				inEdge->mSource->mLastOut = NULL;
			inEdge->mSource->mFirstOut = inEdge->mNextSource;
		}
		else
		{
			if (inEdge->mSource->mLastOut == inEdge)
			{/* deletion at the bottom of the list */
				inEdge->mPrevSource->mNextSource = NULL;
				inEdge->mSource->mLastOut = inEdge->mPrevSource;
			}
			else
			{/* deletion in the middle of the list */
				inEdge->mNextSource->mPrevSource = inEdge->mPrevSource;
				inEdge->mPrevSource->mNextSource = inEdge->mNextSource;
			}
		}
		inEdge->mSource->mOutDeg--;
		inEdge->mTarget->mInDeg--;
		This->mEdgesCount--;
	}
	else
	{ /* ********Undirected Graph Block ************** */
        /* deletes the edge from the list of outEdges of the graph source node*/
		if (inEdge->mSource->mFirstOut == inEdge)
		{/* deletion at the top of the list */
			if(inEdge->mNextSource != NULL) /* there's at least another edge */
				if (inEdge->mNextSource->mSource == inEdge->mSource)
					inEdge->mNextSource->mPrevSource = NULL;
				else
					inEdge->mNextSource->mPrevTarget = NULL;
			else /* there's only this edge in the list, assign NULL to last element's ptr */
				inEdge->mSource->mLastOut = NULL;
			inEdge->mSource->mFirstOut = inEdge->mNextSource;
		}
		else
		if (inEdge->mSource->mLastOut == inEdge)
		{/* deletion at the bottom of the list */
			if (inEdge->mPrevSource->mSource == inEdge->mSource)
				inEdge->mPrevSource->mNextSource = NULL;
			else
				inEdge->mPrevSource->mNextTarget = NULL;
			inEdge->mSource->mLastOut = inEdge->mPrevSource;
		}
		else
		{/* deletion in the middle of the list */
			if (inEdge->mNextSource->mSource == inEdge->mSource) 
				inEdge->mNextSource->mPrevSource = inEdge->mPrevSource;
			else
				inEdge->mNextSource->mPrevTarget = inEdge->mPrevSource;
			
			if (inEdge->mPrevSource->mSource == inEdge->mSource)
				inEdge->mPrevSource->mNextSource = inEdge->mNextSource;
			else
				inEdge->mPrevSource->mNextTarget = inEdge->mNextSource;
		}

		if (inEdge->mSource != inEdge->mTarget)
		{
			/* deletes the edge from the list of outEdges of the graph target node*/
			if (inEdge->mTarget->mFirstOut == inEdge)
			{/* deletion at the top of the list */
				if(inEdge->mNextTarget != NULL) /* there's at least another edge */
					if (inEdge->mNextTarget->mTarget == inEdge->mTarget)
						inEdge->mNextTarget->mPrevTarget = NULL;
					else
						inEdge->mNextTarget->mPrevSource = NULL;
				else /* there's only this edge in the list, assign NULL to last element's ptr */
					inEdge->mTarget->mLastOut = NULL;
				inEdge->mTarget->mFirstOut = inEdge->mNextTarget;
			}
			else
			if (inEdge->mTarget->mLastOut == inEdge)
			{/* deletion at the bottom of the list */
				if (inEdge->mPrevTarget->mTarget == inEdge->mTarget)
					inEdge->mPrevTarget->mNextTarget = NULL;
				else
					inEdge->mPrevTarget->mNextSource = NULL;
				inEdge->mTarget->mLastOut = inEdge->mPrevTarget;
			}
			else
			{/* deletion in the middle of the list */
				if (inEdge->mNextTarget->mTarget == inEdge->mTarget) 
					inEdge->mNextTarget->mPrevTarget = inEdge->mPrevTarget;
				else
					inEdge->mNextTarget->mPrevSource = inEdge->mPrevTarget;
				
				if (inEdge->mPrevTarget->mTarget == inEdge->mTarget)
					inEdge->mPrevTarget->mNextTarget = inEdge->mNextTarget;
				else
					inEdge->mPrevTarget->mNextSource = inEdge->mNextTarget;
			}
		}
		inEdge->mSource->mOutDeg--;
		inEdge->mTarget->mOutDeg--;
		This->mEdgesCount--;
	}

	/* now we must remove the edge info from all registered LEdgeInfo */
	Try
		theCount = LArray_GetItemsCount(This->mEdgeListArray);
	CatchAny
		Rethrow;
	for (i=0; i < theCount; i++)
		if (This->mEdgeList[i])
			_LEdgeInfo_DeleteItemAt(This->mEdgeList[i], inEdge);

	/* now we must remove the edge info from all registered LEdgeMap */
	Try
		theCount = LArray_GetItemsCount(This->mEdgeMapListArray);
	CatchAny
		Rethrow;
	for (i=0; i < theCount; i++)
		if (This->mEdgeMapList[i])
			_LEdgeMap_DeleteEdge(This->mEdgeMapList[i], inEdge);
	LMemory_Free(&inEdge);
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetFirstNode
*  ---------------------------------------------------------------------------------
*  Returns the first node of the graph */
LGraph_TNode* LGraph_GetFirstNode(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mFirstNode; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetLastNode
*  ---------------------------------------------------------------------------------
*  Returns the last node of the graph */
LGraph_TNode* LGraph_GetLastNode(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mLastNode; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetFirstEdge
*  ---------------------------------------------------------------------------------
*  Returns the first edge of the graph */
LGraph_TEdge* LGraph_GetFirstEdge(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mFirstEdge; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetLastEdge
*  ---------------------------------------------------------------------------------
*  Returns the last edge of the graph */
LGraph_TEdge* LGraph_GetLastEdge(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mLastEdge; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetAllNodes
*  ---------------------------------------------------------------------------------
*  Returns a LArray* with the references to all nodes of the graph */
LArray* LGraph_GetAllNodes(LGraph* This)
{
	LArray* theArray;
	LGraph_TNode* theNode;

	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
    
	Try
	{
		theArray = LArray_New( sizeof(LGraph_TNode*) );
		LGraph_ForAllNodes(This, theNode)
			LArray_AppendItem(theArray, &theNode);
	}
	CatchAny
		Rethrow;
	
	return theArray;
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetAllEdges
*  ---------------------------------------------------------------------------------
*  Returns a LArray* with the references to all edges of the graph */
LArray* LGraph_GetAllEdges(LGraph* This)
{
	LArray* theArray;
	LGraph_TEdge* theEdge;

	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);

	Try
	{
		theArray = LArray_New( sizeof(LGraph_TEdge*) );
		LGraph_ForAllEdges(This, theEdge)
			LArray_AppendItem(theArray, &theEdge);
	}
	CatchAny
		Rethrow;

	return theArray;
}

/* ---------------------------------------------------------------------------------
*  LGraph_GetOutEdges
*  ---------------------------------------------------------------------------------
*  Returns a LArray* with the references to out-edges of a node v  */
LArray* LGraph_GetOutEdges(LGraph* This, LGraph_TNode* inNode)
{
	LArray* theArray;
	LGraph_TEdge* theEdge;
	
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inNode->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);
    
	Try
	{
		theArray = LArray_New( sizeof(LGraph_TEdge*) );
		LGraph_ForAllOut(inNode, theEdge)
			LArray_AppendItem(theArray, &theEdge);
	}
	CatchAny
		Rethrow;

	return theArray;
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetInEdges
*  ---------------------------------------------------------------------------------
*  Returns a LArray* with the references to in-edges of a node v, if G is directed *
*  NULL otherwise																   */
LArray* LGraph_GetInEdges(LGraph* This, LGraph_TNode* inNode)
{
	LArray* theArray;
	LGraph_TEdge* theEdge;
		
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inNode->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);

	if (This->mDirected)
	{
		Try
		{
			theArray = LArray_New( sizeof(LGraph_TEdge*) );
			LGraph_ForAllIn(inNode, theEdge)
				LArray_AppendItem(theArray, &theEdge);
		}
		CatchAny
			Rethrow;
		return theArray;
	}
	else
		return NULL;
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetAdjNodes
*  ---------------------------------------------------------------------------------
*  Returns a LArray* with the references to adj nodes of a node v */
LArray* LGraph_GetAdjNodes(LGraph* This, LGraph_TNode* inNode)
{
	LArray* theArray;
	LGraph_TEdge* theEdge;
	
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	if (inNode->mIndex >= This->mNodesCount) Throw(LGraph_NODE_NOT_IN_GRAPH);

	Try
	{
		theArray = LArray_New( sizeof(LGraph_TNode*) );
		LGraph_ForAllOut(inNode, theEdge)
		{ /* if the  graph is  undirected, we  must consider that edges with  source  node *
		   * different from inNode but targeted to inNode, might be the out list of inNode *
		   * so  we  must 'reverse' mSource,mTarget in order to have a correct list of out *
		   * edges   																	   */
			if ( (theEdge->mTarget == inNode) && (theEdge->mSource != inNode) )
				LArray_AppendItem(theArray, &(theEdge->mSource) );
			else
				LArray_AppendItem(theArray, &(theEdge->mTarget) );
		}
	}
	CatchAny
		Rethrow;
	return theArray;
}


/* ---------------------------------------------------------------------------------
*  LGraph_IsDirected, LGraph_IsEmpty
*  ---------------------------------------------------------------------------------
*  Returns basic infos about the graph */
Bool LGraph_IsDirected(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mDirected; 
}
Bool LGraph_IsEmpty(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return (This->mNodesCount == 0); 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetUsedMem
*  ---------------------------------------------------------------------------------
*  Returns the memory usage of the WHOLE structure */
ui4 LGraph_GetUsedMem(LGraph* This)
{
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
    return sizeof(LGraph) + This->mEdgesCount * sizeof(LGraph_TEdge) + 
		   This->mNodesCount * sizeof(LGraph_TNode);
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetInDeg, LGraph_GetOutDeg, LGraph_Degree
*  ---------------------------------------------------------------------------------
*  Simple methods to access node degree infos */
ui4 LGraph_GetInDeg(LGraph_TNode* inNode)
{ 
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	return inNode->mInDeg; 
}
ui4 LGraph_GetOutDeg(LGraph_TNode* inNode)
{ 
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	return inNode->mOutDeg; 
}
ui4 LGraph_GetDegree(LGraph_TNode* inNode)
{
	if (inNode == NULL) Throw(LGraph_NODE_NULL_POINTER);
	return inNode->mInDeg + inNode->mOutDeg; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_GetNodesCount, LGraph_GetEdgesCount
*  ---------------------------------------------------------------------------------
*  Simple methods to access Graph infos */
ui4 LGraph_GetNodesCount(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mNodesCount; 
}
ui4	LGraph_GetEdgesCount(LGraph* This)
{ 
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);
	return This->mEdgesCount; 
}


/* ---------------------------------------------------------------------------------
*  LGraph_Dump
*  ---------------------------------------------------------------------------------
*  Gives a representation of the graph */
void LGraph_Dump(LGraph* This)
{
	LGraph_TEdge* theEdge;
	LGraph_TNode* theNode;

	if (!This->mDebug) return;
	if (This == NULL) Throw(LGraph_OBJECT_NULL_POINTER);

	LDebug_Print("\n\n**************************DUMP*****************************\n");
	LDebug_Print("Nodes    :%u\n", This->mNodesCount);
	LDebug_Print("Edges    :%u\n", This->mEdgesCount);
	LDebug_Print("Memory   :%u\n", LGraph_GetUsedMem(This) );
	LDebug_Print("Directed :%u\n", This->mDirected);
	LDebug_Print("------------------Nodes--------------------\n");
	LGraph_ForAllNodes(This, theNode)
	LDebug_Print("[%u] - [%p] - InDeg:%u - OutDeg:%u\n", theNode->mIndex, theNode,
	theNode->mInDeg, theNode->mOutDeg);
	LDebug_Print("------------------Edges--------------------\n\n");
	LGraph_ForAllEdges(This, theEdge)
	LDebug_Print("[%u] - [%p] - (%u->%u)\n", theEdge->mIndex, theEdge, 
	theEdge->mSource->mIndex, theEdge->mTarget->mIndex);
	LDebug_Print("\n**************************END******************************\n\n");
}

void LGraph_SetDebug(LGraph* This, Bool inDebug)
{ This->mDebug = inDebug; }


/* PRIVATE METHODS */

/* ---------------------------------------------------------------------------------
*  _DeAllocNodeList
*  ---------------------------------------------------------------------------------
*  Deallocates the list of the nodes */
void _DeAllocNodeList(LGraph_TNode* inPtr)
{
	LGraph_TNode* theTemp;

	while (inPtr != NULL)
	{
		theTemp = inPtr;
		inPtr = inPtr->mNext;
		LMemory_Free(&theTemp);
	}
}


/* ---------------------------------------------------------------------------------
*  _DeAllocEdgeList
*  ---------------------------------------------------------------------------------
*  Deallocates the list of the edges */
void _DeAllocEdgeList(LGraph_TEdge* inPtr)
{
	LGraph_TEdge* theTemp;

	while (inPtr != NULL)
	{
		theTemp = inPtr;
		inPtr = inPtr->mNext;
		LMemory_Free(&theTemp);
	}
}


/* ---------------------------------------------------------------------------------
*  _LGraph_RegisterNodeInfo
*  ---------------------------------------------------------------------------------
*  Registers a LNodeInfo */
ui4 _LGraph_RegisterNodeInfo(LGraph* inGraph, LNodeInfo* inNodeInfo)
{ return LArray_AppendItem(inGraph->mNodeListArray, &inNodeInfo); }


/* ---------------------------------------------------------------------------------
*  _LGraph_UnregisterNodeInfo
*  ---------------------------------------------------------------------------------
*  Unregisters a LNodeInfo */
void _LGraph_UnregisterNodeInfo(LGraph* inGraph, ui4 inIdx)
{
	Try
	{*(LNodeInfo**)LArray_ItemAt(inGraph->mNodeListArray, inIdx) = NULL;}
	CatchAny
		Rethrow;
	return;
}


/* ---------------------------------------------------------------------------------
*  _LGraph_RegisterEdgeInfo
*  ---------------------------------------------------------------------------------
*  Registers a LEdgeInfo */
ui4 _LGraph_RegisterEdgeInfo(LGraph* inGraph, LEdgeInfo* inEdgeInfo)
{ return LArray_AppendItem(inGraph->mEdgeListArray, &inEdgeInfo); }


/* ---------------------------------------------------------------------------------
*  _LGraph_UnregisterEdgeInfo
*  ---------------------------------------------------------------------------------
*  Unregisters a LEdgeInfo */
void _LGraph_UnregisterEdgeInfo(LGraph* inGraph, ui4 inIdx)
{

	Try
	{*(LEdgeInfo**)LArray_ItemAt(inGraph->mEdgeListArray, inIdx) = NULL;}
	CatchAny
		Rethrow;
	return;
}

/* ---------------------------------------------------------------------------------
*  _LGraph_RegisterEdgeMap, _LGraph_URegisterEdgeMap
*  ---------------------------------------------------------------------------------
*  Registers or unregisters a LEdgeMap */
ui4 _LGraph_RegisterEdgeMap(LGraph* inGraph, LEdgeMap* inEdgeMap)
{ return LArray_AppendItem(inGraph->mEdgeMapListArray, &inEdgeMap); }

void _LGraph_UnregisterEdgeMap(LGraph* inGraph, ui4 inIdx)
{ 
	Try
	{*(LEdgeMap**)LArray_ItemAt(inGraph->mEdgeMapListArray, inIdx) = NULL;}
	CatchAny
		Rethrow;
	return;
}

/* Copyright (C) 2003 Stefano Emiliozzi, Camil Demetrescu

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

