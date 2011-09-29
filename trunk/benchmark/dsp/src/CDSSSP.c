/* ============================================================================
 *  CDSSSP.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        May  12, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:03 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "CDSSSP.h"
#include "LEdgeMap.h"
#include "LMemory.h"
#include "LException.h"
#include "LGraphGen.h"
#include "_LGraph.h"

#define CDSSSP_INFINITY LType_MAX_UI4
#define CDSSSP_BAD_IDX  LType_MAX_UI4


/* Object Struct */
struct CDSSSP
{
	LHeap*	      mQ;			/* main queue 					     */
	LNodeInfo*    mQIdx;        /* indexes in the main queue          */
	LNodeInfo*    mDist;		/* distance from the source		      */
	LNodeInfo*    mParent;      /* parents nodeinfo                   */
	LNodeInfo*    mSubTreeNode; /* affected subtree's node marker     */
	LEdgeInfo*    mEdgeCost;    /* cost of the edges of the graph     */
	LGraph*	      mGraph;       /* input graph					      */
	LArray*       mGraphNodes;  /* nodes of the graph			      */
	LGraph*	      mSPTree;		/* internal SPTree				      */
	LArray*       mTreeNodes;   /* nodes of the tree			      */
	LArray*       mList0;       /* used to mark subtrees              */
	LArray*       mList1;       /* used to correctly setup main queue */
	LGraph_TNode* mSource;		/* source node of the SP Tree         */
	Bool          mShared;      /* flag used to check if the object 
								   shares structures with others      */
};


/* Private methods declaration */
void          __ModifySPTree(CDSSSP*, LGraph_TNode*);
LGraph_TEdge* __GetEdge(LGraph* inGraph, LGraph_TNode* inSrc, LGraph_TNode* inDst);
void          __MarkSubTreeNodes(CDSSSP*, LGraph_TNode*);
void          __SetupPQueue(CDSSSP*, LGraph_TNode*);
void          CDSSSP_IncreaseEdge(CDSSSP*, LGraph_TEdge*, ui4);
void          CDSSSP_DecreaseEdge(CDSSSP*, LGraph_TEdge*, ui4);

/* Comparator for the Heap */
static Bool _Comp(ui4 inA, ui4 inB) 
{return inA < inB;}

/* ---------------------------------------------------------------------------------
*  NewShared
*  ---------------------------------------------------------------------------------
*  Constructor Shared*/
CDSSSP* CDSSSP_NewShared(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost, 
				         LArray* inGraphNodes, LHeap* inHeap, LArray* inList0, LArray* inList1)
{
	CDSSSP        theObject = {0};
	CDSSSP*       This;
	LGraph_TNode* theGNode;
	LNodeInfo*    theParent    = NULL;
	LNodeInfo*    theDistances = NULL;
    
	Try
	{
		theObject.mGraph    = inGraph;
		theObject.mQ		= inHeap;
		theObject.mEdgeCost = inEdgeCost;
		theObject.mSource   = inSource;
		theObject.mList0    = inList0;
		theObject.mList1    = inList1;
		theObject.mShared   = TRUE;
		theParent           = LNodeInfo_New(inGraph, LType_Ptr);
		theDistances        = LNodeInfo_New(inGraph, LType_UI4);
		This				= LMemory_NewObject(CDSSSP, theObject);
		This->mGraphNodes   = inGraphNodes;
		CSSSP_UI4(This->mGraph, inSource, inEdgeCost, &theDistances, &theParent);
		This->mSPTree       = LGraphGen_BuildShortestPathTree(This->mGraph, theParent, NULL, NULL);
		This->mTreeNodes    = LGraph_GetAllNodes(This->mSPTree);
		This->mDist         = LNodeInfo_New(This->mSPTree, LType_UI4);
		This->mSubTreeNode  = LNodeInfo_New(This->mSPTree, LType_Bool);
		This->mParent       = LNodeInfo_New(This->mSPTree, LType_Ptr);
		This->mQIdx         = LNodeInfo_New(This->mSPTree, LType_UI4);
	}
	CatchAny
	{
		if (This->mSPTree)      LGraph_Delete(&This->mSPTree);
		if (This->mTreeNodes)   LArray_Delete(&This->mTreeNodes);
		if (This->mDist)        LNodeInfo_Delete(&This->mDist);
		if (This->mQIdx)        LNodeInfo_Delete(&This->mQIdx);
		if (This->mSubTreeNode) LNodeInfo_Delete(&This->mSubTreeNode);
		if (theParent)          LNodeInfo_Delete(&theParent);
	    if (theDistances)       LNodeInfo_Delete(&theDistances);
		Rethrow;
	}
	LGraph_ForAllNodes(This->mGraph, theGNode)
	{
		LGraph_TNode* theTNode = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNode->mIndex);
        ui4 theDist = LNodeInfo_UI4At(theDistances, theGNode);
		
		LNodeInfo_UI4At(This->mDist, theTNode)         = theDist;
		LNodeInfo_UI4At(This->mQIdx, theTNode)         = CDSSSP_BAD_IDX;
		LNodeInfo_BoolAt(This->mSubTreeNode, theTNode) = FALSE;
		LNodeInfo_PointerAt(This->mParent, theTNode)   = NULL;
	}
	LNodeInfo_Delete(&theParent);
	LNodeInfo_Delete(&theDistances);
	return This;
}

/* ---------------------------------------------------------------------------------
*  New
*  ---------------------------------------------------------------------------------
*  Constructor */
CDSSSP* CDSSSP_New(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost)
{
	CDSSSP* This;

	Try
	{
		LArray* theGNodes = LGraph_GetAllNodes(inGraph);
		LArray* theList0  = LArray_New(sizeof(LGraph_TNode*));
		LArray* theList1  = LArray_New(sizeof(LGraph_TNode*));
		LHeap* theH = LHeap_New(_Comp);
		This = 	CDSSSP_NewShared(inGraph, inSource, inEdgeCost, theGNodes, theH, theList0, theList1);
		This->mShared = FALSE;
	}
	CatchAny{Rethrow;}

	return This;
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void CDSSSP_Delete(CDSSSP** ThisA)
{
	if (!(*ThisA)->mShared)
	{
		LArray_Delete(&(*ThisA)->mGraphNodes);
		LArray_Delete(&(*ThisA)->mList0);
		LArray_Delete(&(*ThisA)->mList1);
		LHeap_Delete(&(*ThisA)->mQ);
	}
	LNodeInfo_Delete(&(*ThisA)->mDist);
	LNodeInfo_Delete(&(*ThisA)->mQIdx);
	LNodeInfo_Delete(&(*ThisA)->mSubTreeNode);
	LNodeInfo_Delete(&(*ThisA)->mParent);
	LArray_Delete(&(*ThisA)->mTreeNodes);
	LGraph_Delete(&(*ThisA)->mSPTree);
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  GetNodeDistance
*  ---------------------------------------------------------------------------------
*  Returns l(inNode) */
ui4 CDSSSP_GetNodeDistance(CDSSSP* This, LGraph_TNode* inNode)
{ 
	return LNodeInfo_UI4At(This->mDist, inNode); 
}

/* ---------------------------------------------------------------------------------
*  UpdateEdge
*  ---------------------------------------------------------------------------------
*  Updates inEdgeUV and maintains the SPTree */
void CDSSSP_UpdateEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inNewWeight)
{
	ui4 theOldWeight = LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV);

	if (inNewWeight == CDSSSP_INFINITY)
		CDSSSP_IncreaseEdge(This, inEdgeUV, inNewWeight);
	else
	{
		if (inNewWeight > theOldWeight)
			CDSSSP_IncreaseEdge(This, inEdgeUV, inNewWeight - theOldWeight);
		else
			if (inNewWeight < theOldWeight)
				CDSSSP_DecreaseEdge(This, inEdgeUV, theOldWeight - inNewWeight);
	}
}

/* ---------------------------------------------------------------------------------
*  DeleteEdge
*  ---------------------------------------------------------------------------------
*  Deletes inEdgeUV and maintains the SPTree */
void CDSSSP_DeleteEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV)
{	
	LGraph_TEdge* theTEdgeUV;
	LGraph_TNode* theGNodeU = inEdgeUV->mSource;
	LGraph_TNode* theGNodeV = inEdgeUV->mTarget;
	LGraph_TNode* theTNodeU = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeU->mIndex);
	LGraph_TNode* theTNodeV = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeV->mIndex);

	theTEdgeUV = __GetEdge(This->mSPTree, theTNodeU, theTNodeV);
	if ((!This->mGraph->mDirected) && (!theTEdgeUV))
	{/* tries to check the reversed edge */
		theTEdgeUV = __GetEdge(This->mSPTree, theTNodeV, theTNodeU);
	}
	if (!theTEdgeUV)
	{/* it's not a tree edge */
		if (!This->mShared)
			LGraph_DelEdge(This->mGraph, inEdgeUV);
		return;
	}
	if (!This->mShared)
		LGraph_DelEdge(This->mGraph, inEdgeUV);
	theTNodeV = theTEdgeUV->mTarget;
	/* deletes the edge in the Tree */
	LGraph_DelEdge(This->mSPTree, theTEdgeUV);
	/* marks the subtree(inNodeV) */
	__MarkSubTreeNodes(This, theTNodeV);
	/* loads the queue only with nodes in subtree(inNodeV) */
	__SetupPQueue(This, theTNodeV);
	/* maintains SPTree */
	__ModifySPTree(This, theTNodeV);
}

/* ---------------------------------------------------------------------------------
*  IncreaseEdge
*  ---------------------------------------------------------------------------------
*  Increases inEdgeUV and maintains the SPTree */
void CDSSSP_IncreaseEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inDelta)
{	
	LGraph_TEdge*  theTEdgeUV;
	LGraph_TNode*  theGNodeU  = inEdgeUV->mSource;
	LGraph_TNode*  theGNodeV  = inEdgeUV->mTarget;
	LGraph_TNode* theTNodeU   = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeU->mIndex);
	LGraph_TNode* theTNodeV   = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeV->mIndex);
	

	if (!This->mShared)
		LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV) += inDelta;
		
	theTEdgeUV = __GetEdge(This->mSPTree, theTNodeU, theTNodeV);
	if ((!This->mGraph->mDirected) && (!theTEdgeUV))
	{/* tries to check the reversed edge */
		theTEdgeUV = __GetEdge(This->mSPTree, theTNodeV, theTNodeU);
	}
	if (!theTEdgeUV)/* it's not a tree edge */
		return;
	theTNodeU = theTEdgeUV->mSource;
	theTNodeV = theTEdgeUV->mTarget;
	/* marks the subtree(inNodeV) */
	__MarkSubTreeNodes(This, theTNodeV);
	/* loads the queue only with nodes in subtree(inNodeV) */
	__SetupPQueue(This, theTNodeV);
	/* maintains SPTree */
	__ModifySPTree(This, theTNodeV);
}


/* ---------------------------------------------------------------------------------
*  DecreaseEdge
*  ---------------------------------------------------------------------------------
*  Decreases inEdgeUV and maintains the SPTree */
void CDSSSP_DecreaseEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inDelta)
{	
	ui4 theDist, theWeight, theKey, theIdx;
	LGraph_TNode* theGNodeU = inEdgeUV->mSource;
	LGraph_TNode* theGNodeV = inEdgeUV->mTarget;
	LGraph_TNode* theTNodeU = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeU->mIndex);
	LGraph_TNode* theTNodeV = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGNodeV->mIndex);
	
	if (!This->mShared)
	{
		ui4 theOldWeight = LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV);

		if (inDelta > theOldWeight)
			inDelta = theOldWeight;
		LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV) -= inDelta;
	}
	if (!This->mGraph->mDirected)
	{/* undirect graph code */
		if (LNodeInfo_UI4At(This->mDist, theTNodeU) > LNodeInfo_UI4At(This->mDist, theTNodeV))
		{
			LGraph_TNode* theSwap = theTNodeU;

			theTNodeU = theTNodeV;
			theTNodeV = theSwap;
		}
	}

	theDist   = LNodeInfo_UI4At(This->mDist, theTNodeU);
	theWeight = LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV);
	theKey    = theDist + theWeight;
	if (theDist == CDSSSP_INFINITY)
		theKey = CDSSSP_INFINITY;
	if (theKey >= LNodeInfo_UI4At(This->mDist, theTNodeV))
		return;
	
	LNodeInfo_PointerAt(This->mParent, theTNodeV) = theTNodeU;
	theIdx = LHeap_Add(This->mQ, (const void*)theTNodeV, theKey);
	LNodeInfo_UI4At(This->mQIdx, theTNodeV) = theIdx;
	/* maintains SPTree */
	__ModifySPTree(This, theTNodeV);
}

/* ---------------------------------------------------------------------------------
*  InsertEdge
*  ---------------------------------------------------------------------------------
*  Inserts inEdgeUV with weight inWeight and maintains the SPTree */
void CDSSSP_InsertEdge(CDSSSP* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg, ui4 inWeight)
{	
	ui4 theDist, theKey, theIdx;
	LGraph_TEdge* theEdgeUV;
	LGraph_TNode* theTNodeU = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, inSrc->mIndex);
	LGraph_TNode* theTNodeV = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, inTrg->mIndex);
	
	if (!This->mGraph->mDirected)
		if (LNodeInfo_UI4At(This->mDist, theTNodeU) > LNodeInfo_UI4At(This->mDist, theTNodeV))
		{
			LGraph_TNode* theSwap = theTNodeU;

			theTNodeU = theTNodeV;
			theTNodeV = theSwap;
		}

	if (!This->mShared)
		theEdgeUV = LGraph_NewEdge(This->mGraph, inSrc, inTrg);

	LEdgeInfo_UI4At(This->mEdgeCost, theEdgeUV) = inWeight;
	theDist      = LNodeInfo_UI4At(This->mDist, theTNodeU);
	theKey       = theDist + inWeight;
	if (theDist == CDSSSP_INFINITY)
		theKey = CDSSSP_INFINITY;

	if (theKey >= LNodeInfo_UI4At(This->mDist, theTNodeV))
		return;
	
	LNodeInfo_PointerAt(This->mParent, theTNodeV) = theTNodeU;
	theIdx = LHeap_Add(This->mQ, (const void*)theTNodeV, theKey);
	LNodeInfo_UI4At(This->mQIdx, theTNodeV) = theIdx;
	/* maintains SPTree */
	__ModifySPTree(This, theTNodeV);
}

/* ---------------------------------------------------------------------------------
*  GetSourceNode
*  ---------------------------------------------------------------------------------
*  Returns the source node of the SPTree */
LGraph_TNode* CDSSSP_GetSourceNode(CDSSSP* This)
{return This->mSource;}

/* ---------------------------------------------------------------------------------
*  GetNodeParent
*  ---------------------------------------------------------------------------------
*  Returns the parent of the GRAPH node inNode */
LGraph_TNode* CDSSSP_GetNodeParent(CDSSSP* This, LGraph_TNode* inNode)
{
	LGraph_TNode* theTreeNode =  *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, inNode->mIndex);
	LGraph_TEdge* theParentEdge =  theTreeNode->mFirstIn;

	if (!theParentEdge)
		return NULL;
	else
	{
		LGraph_TNode* theTreeParentNode = theParentEdge->mSource;
		return *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theTreeParentNode->mIndex);
	}
}

/* ---------------------------------------------------------------------------------
*  GetUsedMem
*  ---------------------------------------------------------------------------------
*  Returns total memory usage of the object */
ui4 CDSSSP_GetUsedMem(CDSSSP* This)
{
	ui4 theUsedMem = 0;
 
	theUsedMem += sizeof(CDSSSP);
	theUsedMem += LNodeInfo_GetUsedMem(This->mDist);
	theUsedMem += LNodeInfo_GetUsedMem(This->mQIdx);
	theUsedMem += LArray_GetUsedMem(This->mTreeNodes);
	theUsedMem += LGraph_GetUsedMem(This->mSPTree);
	theUsedMem += LNodeInfo_GetUsedMem(This->mSubTreeNode);
	theUsedMem += LNodeInfo_GetUsedMem(This->mParent);
	if (!This->mShared)
	{
		theUsedMem += LArray_GetUsedMem(This->mGraphNodes);
		theUsedMem += LArray_GetUsedMem(This->mList0);
		theUsedMem += LArray_GetUsedMem(This->mList1);
		theUsedMem += LHeap_GetUsedMem(This->mQ);
	}
	return theUsedMem;
}


/*Private methods definition */

/* ---------------------------------------------------------------------------------
*  ModifySPTree
*  ---------------------------------------------------------------------------------
*  Modifies the shortest path tree, according to an edge removal */
void __ModifySPTree(CDSSSP* This, LGraph_TNode* inNodeV)
{
	while (!LHeap_Empty(This->mQ))
	{
		LGraph_TEdge* theGEdge;
		LGraph_TNode  *theTNode, *theGNode, *theParent;
		ui4           theKey;

		LHeap_ExtractMin(This->mQ, (void**)&theTNode, (ui4*)&theKey);
		LNodeInfo_UI4At(This->mQIdx, theTNode) = CDSSSP_BAD_IDX;
		/* updates the distance */
		LNodeInfo_UI4At(This->mDist, theTNode) = theKey;
		/* deletes old edge */
		if (theTNode->mFirstIn)
			LGraph_DelEdge(This->mSPTree, theTNode->mFirstIn);
		/* retrives the best parent */
		theParent = (LGraph_TNode*)LNodeInfo_PointerAt(This->mParent, theTNode);
		LNodeInfo_PointerAt(This->mParent, theTNode) = NULL;
		/* attaches theTNode to the new parent */
		LGraph_NewEdge(This->mSPTree, theParent, theTNode);
		/* cleans the subtree mark */
		LNodeInfo_BoolAt(This->mSubTreeNode, theTNode) = FALSE;
		/* gets matching graph node */
		theGNode = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theTNode->mIndex);
		LGraph_ForAllOut(theGNode, theGEdge)
		{
			LGraph_TNode* theGTrg = 
				(theGNode == theGEdge->mTarget)? theGEdge->mSource : theGEdge->mTarget;
			LGraph_TNode* theTTrg = 
				*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGTrg->mIndex);
			ui4 theWeight = LEdgeInfo_UI4At(This->mEdgeCost, theGEdge);
			ui4 theAdjDist= theKey + theWeight;
			
			if ( (theKey == CDSSSP_INFINITY) || (theWeight == CDSSSP_INFINITY) )
				theAdjDist = CDSSSP_INFINITY;

			if ( theAdjDist < LNodeInfo_UI4At(This->mDist, theTTrg))
			{
				if (LNodeInfo_UI4At(This->mQIdx, theTTrg) != CDSSSP_BAD_IDX)
				{
					/* relaxation */
					ui4 theIdx = LNodeInfo_UI4At(This->mQIdx, theTTrg);
					LHeap_Update(This->mQ, (const void*)theTTrg, theAdjDist, theIdx);
					LNodeInfo_UI4At(This->mDist, theTTrg) = theAdjDist;
					LNodeInfo_PointerAt(This->mParent, theTTrg) = theTNode;
				}
				else
				{
					ui4 theIdx = LHeap_Add(This->mQ, (const void*)theTTrg, theAdjDist);
					LNodeInfo_UI4At(This->mQIdx, theTTrg) = theIdx;
					LNodeInfo_UI4At(This->mDist, theTTrg) = theAdjDist;
					LNodeInfo_PointerAt(This->mParent, theTTrg) = theTNode;
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  _GetEdge
*  ---------------------------------------------------------------------------------
*  Gets the edge between two nodes */
LGraph_TEdge* __GetEdge(LGraph* inGraph, LGraph_TNode* inSrc, LGraph_TNode* inDst)
{
	LGraph_TEdge* theEdge;

	LGraph_ForAllOut(inSrc, theEdge)
		if (theEdge->mTarget == inDst)
			return theEdge;
	return NULL;
}

/* ---------------------------------------------------------------------------------
*  __MarkSubTreeNodes
*  ---------------------------------------------------------------------------------
*  Marks subtree(inNode) */
void __MarkSubTreeNodes(CDSSSP* This, LGraph_TNode* inNode)
{
	LArray_AppendItem(This->mList0, (const void*)&inNode);
	while (LArray_GetItemsCount(This->mList0))
	{
		LGraph_TEdge* theEdge;
		LGraph_TNode* theNode = *(LGraph_TNode**)LArray_LastItem(This->mList0);
		LArray_RemoveLastItem(This->mList0);//dequeues from the queue
		LNodeInfo_BoolAt(This->mSubTreeNode, theNode) = !LNodeInfo_BoolAt(This->mSubTreeNode, theNode);
		LGraph_ForAllOut(theNode, theEdge)
			/* enqueues the sons of the theNode */
			LArray_AppendItem(This->mList0, (const void*)&(theEdge->mTarget));
	}
}

/* ---------------------------------------------------------------------------------
*  __SetupPQueue
*  ---------------------------------------------------------------------------------
*  Finds the best parent for each marked node */
void __SetupPQueue(CDSSSP* This, LGraph_TNode* inNode)
{
	LArray_AppendItem(This->mList1, (const void*)&inNode);
	while (LArray_GetItemsCount(This->mList1))
	{
		ui4 theIdx;
		ui4 theMin  = CDSSSP_INFINITY;
		LGraph_TEdge* theTEdge;
		LGraph_TNode* theBestNode = NULL;
		LGraph_TNode *theTNode, *theGNode;
		
		theTNode = *(LGraph_TNode**)LArray_LastItem(This->mList1);
		LArray_RemoveLastItem(This->mList1);//dequeues from the queue
		theGNode = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theTNode->mIndex);
		if (This->mGraph->mDirected)
		{/* directed graph block */
			LGraph_TEdge* theGEdge;

			LGraph_ForAllIn(theGNode, theGEdge)
			{
				LGraph_TNode* theTSrc = 
					*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGEdge->mSource->mIndex);
				
				if (!LNodeInfo_BoolAt(This->mSubTreeNode, theTSrc))
				{/* if the node isn't in the modified subtree */
					ui4 theWeight = LEdgeInfo_UI4At(This->mEdgeCost, theGEdge);
					ui4 theDist   = LNodeInfo_UI4At(This->mDist, theTSrc);
					ui4 theKey    = theDist + theWeight;

					if ( (theDist == CDSSSP_INFINITY) || (theWeight == CDSSSP_INFINITY) )
						theKey = CDSSSP_INFINITY;

					if (theKey < theMin)
					{
						theMin = theKey;
						theBestNode = theTSrc;
					}
				}
			}
		}
		else
		{/* undirected graph block */
			LGraph_TEdge* theGEdge;

			LGraph_ForAllOut(theGNode, theGEdge)
			{
				LGraph_TNode* theTSrc = 
					*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGEdge->mSource->mIndex);

				if (theTSrc->mIndex == theGNode->mIndex)
					theTSrc = 
						*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGEdge->mTarget->mIndex);

				if (!LNodeInfo_BoolAt(This->mSubTreeNode, theTSrc))
				{	/* if the node isn't in the modified subtree */
					ui4 theWeight = LEdgeInfo_UI4At(This->mEdgeCost, theGEdge);
					ui4 theDist   = LNodeInfo_UI4At(This->mDist, theTSrc);
					ui4 theKey    = theDist + theWeight;

					if ( (theDist == CDSSSP_INFINITY) || (theWeight == CDSSSP_INFINITY) )
						theKey = CDSSSP_INFINITY;

					if (theKey < theMin)
					{
						theMin = theKey;
						theBestNode = theTSrc;
					}
				}
			}
		}
		
		if (theMin < CDSSSP_INFINITY)
		{
			/* saves info about the best parent */
			LNodeInfo_PointerAt(This->mParent, theTNode) = theBestNode;
			/* enqueues the node in the main queue */
			theIdx = LHeap_Add(This->mQ, (const void*)theTNode, theMin);
			LNodeInfo_UI4At(This->mQIdx, theTNode) = theIdx;
			/* cleans the subtree's mark */
			LNodeInfo_BoolAt(This->mSubTreeNode, theTNode) = FALSE;
		}
		/* updates its distance */
		LNodeInfo_UI4At(This->mDist, theTNode) = theMin;
		/* enqueues its sons */
		LGraph_ForAllOut(theTNode, theTEdge)
			LArray_AppendItem(This->mList1, (const void*)&theTEdge->mTarget);
	}
}

/* Copyright (C) 2003 Stefano Emiliozzi 

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

