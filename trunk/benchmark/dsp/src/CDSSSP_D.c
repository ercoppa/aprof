/* ============================================================================
 *  CDSSSP_D.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 17, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:03 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "CDSSSP_D.h"
#include "LEdgeMap.h"
#include "LMemory.h"
#include "LException.h"
#include "LGraphGen.h"
#include "_LGraph.h"

#define CDSSSP_D_INFINITY LType_MAX_UI4
/* Private methods declaration */
void          _ModifySPTree(CDSSSP_D*, LGraph_TNode*);
LGraph_TEdge* _GetEdge(LGraph* inGraph, LGraph_TNode* inSrc, LGraph_TNode* inDst);
LGraph_TEdge* _GetWitness(CDSSSP_D*, LGraph_TNode*, ui4);    
void          _DropsNodesLeft(CDSSSP_D*);

/* ---------------------------------------------------------------------------------
*  New
*  ---------------------------------------------------------------------------------
*  Constructor */
CDSSSP_D* CDSSSP_D_New(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost, 
					   LArray* inGraphNodes, LArray* inArray, ui4 inD)
{
	CDSSSP_D      theObject = {0};
	CDSSSP_D*     This;
	LGraph_TNode* theGraphNode;
	LNodeInfo*    theParent    = NULL;
	LNodeInfo*    theDistances = NULL;
    
	Try
	{
		theObject.mGraph    = inGraph;
		theObject.mQ		= inArray;
		theObject.mEdgeCost = inEdgeCost;
		theObject.mDValue	= inD;
		theObject.mSource   = inSource;
		theParent           = LNodeInfo_New(inGraph, LType_Ptr);
		theDistances        = LNodeInfo_New(inGraph, LType_UI4);
		This				= LMemory_NewObject(CDSSSP_D, theObject);
		This->mGraphNodes   = inGraphNodes;
		CSSSP_UI4(This->mGraph, inSource, inEdgeCost, &theDistances, &theParent);
		This->mSPTree       = LGraphGen_BuildShortestPathTree(This->mGraph, theParent, NULL, NULL);
		This->mTreeNodes    = LGraph_GetAllNodes(This->mSPTree);
		This->mL            = LNodeInfo_New(This->mSPTree, LType_UI4);
		This->mUnsettled    = LNodeInfo_New(This->mSPTree, LType_Bool);
	}
	CatchAny
	{
		if (This->mSPTree)      LGraph_Delete(&This->mSPTree);
		if (This->mTreeNodes)   LArray_Delete(&This->mTreeNodes);
		if (This->mL)           LNodeInfo_Delete(&This->mL);
		if (This->mUnsettled)   LNodeInfo_Delete(&This->mUnsettled);
		if (theParent)          LNodeInfo_Delete(&theParent);
	    if (theDistances)       LNodeInfo_Delete(&theDistances);
		Rethrow;
	}
	LGraph_ForAllNodes(This->mGraph, theGraphNode)
	{
		LGraph_TNode* theTreeNode = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphNode->mIndex);
        ui4 theDist = LNodeInfo_UI4At(theDistances, theGraphNode);
		if (theDist <= This->mDValue)
			LNodeInfo_UI4At(This->mL, theTreeNode) = theDist;
		else
		{
			LNodeInfo_UI4At(This->mL, theTreeNode) = CDSSSP_D_INFINITY;
			if (theTreeNode->mFirstIn)
				LGraph_DelEdge(This->mSPTree, theTreeNode->mFirstIn);
		}
		LNodeInfo_BoolAt(This->mUnsettled, theTreeNode) = FALSE;
	}
	LNodeInfo_Delete(&theParent);
	LNodeInfo_Delete(&theDistances);
	return This;
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void CDSSSP_D_Delete(CDSSSP_D** ThisA)
{
	LNodeInfo_Delete(&(*ThisA)->mL);
	LNodeInfo_Delete(&(*ThisA)->mUnsettled);
	LArray_Delete(&(*ThisA)->mTreeNodes);
	LGraph_Delete(&(*ThisA)->mSPTree);
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  GetNodeDistance
*  ---------------------------------------------------------------------------------
*  Returns l(inNode) 
ui4 CDSSSP_D_GetNodeDistance(CDSSSP_D* This, Node* inNode)
{ 
	ui4 theDist = LNodeInfo_UI4At(This->mL, inNode); 
	if (theDist > This->mDValue)
		return INFINITY;
	else
		return theDist;
}
*/


/* ---------------------------------------------------------------------------------
*  DeleteEdge
*  ---------------------------------------------------------------------------------
*  Deletes inEdgeUV and maintains the SPTree */
void CDSSSP_D_DeleteEdge(CDSSSP_D* This, LGraph_TEdge* inEdgeUV)
{	
	LGraph_TEdge* theTreeEdgeUV;
	LGraph_TNode* theGraphNodeU = inEdgeUV->mSource;
	LGraph_TNode* theGraphNodeV = inEdgeUV->mTarget;
	LGraph_TNode* theTreeNodeU  = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphNodeU->mIndex);
	LGraph_TNode* theTreeNodeV  = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphNodeV->mIndex);

	theTreeEdgeUV = _GetEdge(This->mSPTree, theTreeNodeU, theTreeNodeV);
	if ((!This->mGraph->mDirected) && (!theTreeEdgeUV))
	{/* tries to check the reversed edge */
		theTreeEdgeUV = _GetEdge(This->mSPTree, theTreeNodeV, theTreeNodeU);
	}
	if (!theTreeEdgeUV)
	{/* it's not a tree edge */
		/* deletes the edge in the Graph */
		LGraph_DelEdge(This->mGraph, inEdgeUV);
		return;
	}
	theTreeNodeV = theTreeEdgeUV->mTarget;
	/* deletes the edge in the Tree */
	LGraph_DelEdge(This->mSPTree, theTreeEdgeUV);
	/* unsettles the node */
	LNodeInfo_BoolAt(This->mUnsettled, theTreeNodeV) = TRUE;
	/* maintains SPTree */
	_ModifySPTree(This, theTreeNodeV);
	/* cleans the queue */
	_DropsNodesLeft(This);
}

/* ---------------------------------------------------------------------------------
*  IncreaseEdge
*  ---------------------------------------------------------------------------------
*  Increases inEdgeUV and maintains the SPTree */
void CDSSSP_D_IncreaseEdge(CDSSSP_D* This, LGraph_TEdge* inEdgeUV, ui4 inDelta)
{	
	LGraph_TEdge*  theTreeEdgeUV;
	LGraph_TNode*  theGraphNodeU = inEdgeUV->mSource;
	LGraph_TNode*  theGraphNodeV = inEdgeUV->mTarget;
	LGraph_TNode* theTreeNodeU  = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphNodeU->mIndex);
	LGraph_TNode* theTreeNodeV  = *(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphNodeV->mIndex);
	

	theTreeEdgeUV = _GetEdge(This->mSPTree, theTreeNodeU, theTreeNodeV);
	if ((!This->mGraph->mDirected) && (!theTreeEdgeUV))
	{/* tries to check the reversed edge */
		theTreeEdgeUV = _GetEdge(This->mSPTree, theTreeNodeV, theTreeNodeU);
	}
	if (!theTreeEdgeUV)/* it's not a tree edge */
		return;

	theTreeNodeU = theTreeEdgeUV->mSource;
	theTreeNodeV = theTreeEdgeUV->mTarget;
	/* removes edge (u,v) from Tree */
	LGraph_DelEdge(This->mSPTree, theTreeEdgeUV);
	LNodeInfo_BoolAt(This->mUnsettled, theTreeNodeV) = TRUE;
	/* maintains SPTree */
	_ModifySPTree(This, theTreeNodeV);
	/* cleans the queue */
	_DropsNodesLeft(This);
}

/* ---------------------------------------------------------------------------------
*  GetSourceNode
*  ---------------------------------------------------------------------------------
*  Returns the source node of the SPTree */
LGraph_TNode* CDSSSP_D_GetSourceNode(CDSSSP_D* This)
{return This->mSource;}

/* ---------------------------------------------------------------------------------
*  GetNodeParent
*  ---------------------------------------------------------------------------------
*  Returns the parent of the GRAPH node inNode */
LGraph_TNode* CDSSSP_D_GetNodeParent(CDSSSP_D* This, LGraph_TNode* inNode)
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
ui4 CDSSSP_D_GetUsedMem(CDSSSP_D* This)
{
	ui4 theUsedMem = 0;
 
	theUsedMem += sizeof(CDSSSP_D);
	theUsedMem += LNodeInfo_GetUsedMem(This->mL);
	theUsedMem += LArray_GetUsedMem(This->mTreeNodes);
	theUsedMem += LGraph_GetUsedMem(This->mSPTree);
	theUsedMem += LNodeInfo_GetUsedMem(This->mUnsettled);
	return theUsedMem;
}


/*Private methods definition */

/* ---------------------------------------------------------------------------------
*  ModifySPTree
*  ---------------------------------------------------------------------------------
*  Modifies the shortest path tree, according to an edge removal */
void _ModifySPTree(CDSSSP_D* This, LGraph_TNode* inNodeV)
{
	/* i <- ds(inNodeV) */
	ui4 i = LNodeInfo_UI4At(This->mL, inNodeV);

	if (i < CDSSSP_D_INFINITY) 
		LArray_AppendItem(This->mQ, (const void*)&inNodeV);
	while ( (i < This->mDValue) && (LArray_GetItemsCount(This->mQ)) )
	{
		i4 j;
		i4 theCnt = (i4)LArray_GetItemsCount(This->mQ);

		for (j=0; j < theCnt; j++)
		{
			LGraph_TNode* theNode = *(LGraph_TNode**)LArray_ItemAt(This->mQ, j);
			if ( (theNode->mInDeg == 0) && (LNodeInfo_UI4At(This->mL, theNode) == i) )
			{
				ui4 k;
				LArray* theOut; 
				ui4 theEdgesCnt = theNode->mOutDeg; 
				LGraph_TEdge* theWitness;
				
				theOut = (theEdgesCnt)? LGraph_GetOutEdges(This->mSPTree, theNode) : NULL;
				
				/* removes all out(theNode) */
				for (k=0; k < theEdgesCnt; k++)
				{
					LGraph_TEdge* theEdge = *(LGraph_TEdge**)LArray_ItemAt(theOut, k);
					LGraph_TNode* theTrg  = theEdge->mTarget;
                    					
					LGraph_DelEdge(This->mSPTree, theEdge);
					LArray_AppendItem(This->mQ, (const void*)&theTrg);
					LNodeInfo_BoolAt(This->mUnsettled, theTrg) = TRUE;
				}
				if (theOut)
					LArray_Delete(&theOut);
				theWitness = _GetWitness(This, theNode, i);
				if (theWitness)
				{
					LGraph_TNode* theGraphSrc = theWitness->mSource;
					LGraph_TNode* theGraphTrg = theWitness->mTarget;
					LGraph_TNode* theTreeSrc  = 
						*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphSrc->mIndex);
					
					if (theTreeSrc == theNode)
					{/* reversed edge due to undirected graph */
						theTreeSrc  = 
							*(LGraph_TNode**)LArray_ItemAt(This->mTreeNodes, theGraphTrg->mIndex);
					}
					LGraph_NewEdge(This->mSPTree, theTreeSrc, theNode);
					LNodeInfo_BoolAt(This->mUnsettled, theNode) = FALSE;
					if (This->mCallBack)
					{
						LGraph_TNode* theGNode = 
							*(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theNode->mIndex);

						This->mCallBack(This->mCDAPSP_D, theGNode);
					}
					/* removes the node from the queue */
					LArray_RemoveItemAt(This->mQ, j);
					j--;
					theCnt--;
                }
				else
					/* ds(theNode) <- ds(theNode) + 1 */
					LNodeInfo_UI4At(This->mL, theNode)++;
             }
		}
		i++;
	}
}

/* ---------------------------------------------------------------------------------
*  _InstallCallBack
*  ---------------------------------------------------------------------------------
*  Installs a CallBack function which is called when a node changes its distance */
void CDSSSP_D_InstallCallBack(CDSSSP_D* This, CDAPSP_D* inCDAPSP_D,
							  void (*inCallBack)(CDAPSP_D*, LGraph_TNode*))
{
	This->mCallBack = inCallBack;
	This->mCDAPSP_D = inCDAPSP_D;
}

/* ---------------------------------------------------------------------------------
*  _GetEdge
*  ---------------------------------------------------------------------------------
*  Gets the edge between two nodes */
LGraph_TEdge* _GetEdge(LGraph* inGraph, LGraph_TNode* inSrc, LGraph_TNode* inDst)
{
	LGraph_TEdge* theEdge;

	LGraph_ForAllOut(inSrc, theEdge)
		if (theEdge->mTarget == inDst)
			return theEdge;
	return NULL;
}

/* ---------------------------------------------------------------------------------
*  _GetWitness
*  ---------------------------------------------------------------------------------
*  Returns a new witness for the node if this exists */
LGraph_TEdge* _GetWitness(CDSSSP_D* This, LGraph_TNode* inTreeNode, ui4 inValue)
{
	LGraph_TEdge* theEdge;
	LGraph_TNode* theGraphNode = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, inTreeNode->mIndex);
	ui4   theWeight, theSrcDist, theTrgDist;

	
	if (This->mGraph->mDirected)
	{
		LGraph_ForAllIn(theGraphNode, theEdge)
		{
			LGraph_TNode* theSrc = theEdge->mSource;
			LGraph_TNode* theTrg = theEdge->mTarget;
			Bool  theSet = !LNodeInfo_BoolAt(This->mUnsettled, theSrc);
		
			theSrcDist = LNodeInfo_UI4At(This->mL, theSrc);
			theTrgDist = LNodeInfo_UI4At(This->mL, theTrg);
			theWeight  = LNodeInfo_UI4At(This->mEdgeCost, theEdge);
			if (theWeight < CDSSSP_D_INFINITY)
				if ( (theSrcDist <= inValue) && (theSrcDist + theWeight == theTrgDist)  && theSet)
					return theEdge;
		}
	}
	else
	{
		LGraph_ForAllOut(theGraphNode, theEdge)
		{
			LGraph_TNode* theSrc = theEdge->mSource;
			LGraph_TNode* theTrg = theEdge->mTarget;
			Bool  theSet;
		
			if (theSrc == theGraphNode)
			{
				/* reversed edge due to undirected graph */
				theSrc = theEdge->mTarget;
				theTrg = theEdge->mSource;
			}
			theSrcDist = LNodeInfo_UI4At(This->mL, theSrc);
			theTrgDist = LNodeInfo_UI4At(This->mL, theTrg);
			theWeight  = LNodeInfo_UI4At(This->mEdgeCost, theEdge);
			theSet     = !LNodeInfo_BoolAt(This->mUnsettled, theSrc);
			if (theWeight < CDSSSP_D_INFINITY)
				if ( (theSrcDist <= inValue) && (theSrcDist + theWeight == theTrgDist) && theSet)
					return theEdge;
		}
	}
	return NULL;
}

/* ---------------------------------------------------------------------------------
*  _DropsNodesLeft
*  ---------------------------------------------------------------------------------
*  Assigns INFINITY to node whose distance is bigger than D */
void _DropsNodesLeft(CDSSSP_D* This)
{
	ui4 i;
	ui4 theCnt = LArray_GetItemsCount(This->mQ);

	for (i=0; i < theCnt; i++)
	{
		LGraph_TNode* theNode  = *(LGraph_TNode**)LArray_ItemAt(This->mQ, i);
		LGraph_TNode* theGNode = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theNode->mIndex);

		LNodeInfo_UI4At(This->mL, theNode) = CDSSSP_D_INFINITY;
		if (This->mCallBack)
			This->mCallBack(This->mCDAPSP_D, theGNode);
	}
	LArray_RemoveAllItems(This->mQ);
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

