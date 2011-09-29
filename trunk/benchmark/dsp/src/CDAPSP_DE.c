/* ============================================================================
 *  CDAPSP_DE.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        May 13, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:02 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "CDAPSP_DE.h"
#include "_CDSSSP.h"
#include "LDebug.h"
#include "LException.h"
#include "LEdgeMap.h"
#include "LMemory.h"

#define CDAPSP_DE_INFINITY LType_MAX_UI4
struct CDAPSP_DE
{
	LNodeInfo* mSSSP;
	LHeap*     mHeap;
	LArray*    mArray0;
	LArray*    mArray1;
	LGraph*    mGraph;
	LEdgeInfo* mEdgeCost;
	LArray*    mGraphNodes;

};

/* Comparator for the Heap */
static Bool _Comp(ui4 inA, ui4 inB) 
{return inA < inB;}

/* private methods declarations */
void _DeallocSSSP(LNodeInfo*, LGraph_TNode*);

/* ---------------------------------------------------------------------------------
*  Constructor
*  --------------------------------------------------------------------------------- */
CDAPSP_DE* CDAPSP_DE_New(LGraph* inGraph, LEdgeInfo* inEdgeCost)
{
	LGraph_TNode* theNode;
	CDAPSP_DE theObject = {0};
	CDAPSP_DE* This;

	Try
	{
		theObject.mSSSP       = LNodeInfo_New(inGraph, LType_Ptr);
		theObject.mGraph      = inGraph;
		theObject.mEdgeCost   = inEdgeCost;
		theObject.mHeap       = LHeap_New(_Comp);
		theObject.mArray0     = LArray_New(sizeof(LGraph_TNode*));
		theObject.mArray1     = LArray_New(sizeof(LGraph_TNode*));
		theObject.mGraphNodes = LGraph_GetAllNodes(inGraph);
	}
	CatchAny
	{
		if (theObject.mSSSP)       LNodeInfo_Delete(&theObject.mSSSP);
		if (theObject.mHeap)       LHeap_Delete(&theObject.mHeap);
		if (theObject.mGraphNodes) LArray_Delete(&theObject.mGraphNodes);
		if (theObject.mArray0)     LArray_Delete(&theObject.mArray0);
		if (theObject.mArray1)     LArray_Delete(&theObject.mArray1);
		Rethrow;
	}
    This = LMemory_NewObject(CDAPSP_DE, theObject);
	LGraph_ForAllNodes(inGraph, theNode)
        LNodeInfo_PointerAt(This->mSSSP, theNode) = 
			CDSSSP_NewShared(inGraph, theNode, inEdgeCost, This->mGraphNodes, This->mHeap, 
				This->mArray0, This->mArray1);

	LNodeInfo_InstallDelNodeHandler(This->mSSSP, _DeallocSSSP);
	return This;
}

/* ---------------------------------------------------------------------------------
*  Destructor
*  ---------------------------------------------------------------------------------
*  Deallocates the object */
void CDAPSP_DE_Delete(CDAPSP_DE** ThisA)
{
	LNodeInfo_Delete(&(*ThisA)->mSSSP);
	LHeap_Delete(&(*ThisA)->mHeap);
	LArray_Delete(&(*ThisA)->mArray0);
	LArray_Delete(&(*ThisA)->mArray1);
	LArray_Delete(&(*ThisA)->mGraphNodes);
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  UpdateEdge
*  ---------------------------------------------------------------------------------
*  Side Effect inEdgeCost, decreases the edge in all CDSSSP */
void CDAPSP_DE_UpdateEdge(CDAPSP_DE* This, LGraph_TEdge* inEdgeUV, ui4 inNewWeight)
{
	LGraph_TNode* theNode;
	ui4 theOldWeight = LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV);
	
	LEdgeInfo_UI4At(This->mEdgeCost, inEdgeUV) = inNewWeight;
	if (theOldWeight == inNewWeight) 
		return;

	if (inNewWeight == CDAPSP_DE_INFINITY)
	{
		LGraph_ForAllNodes(This->mGraph, theNode)
		{
			CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
			CDSSSP_IncreaseEdge(theSPTree, inEdgeUV, inNewWeight);
		}
		
		return;
	}

	if (theOldWeight > inNewWeight)
		LGraph_ForAllNodes(This->mGraph, theNode)
		{
			CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
			CDSSSP_DecreaseEdge(theSPTree, inEdgeUV, theOldWeight - inNewWeight);
		}
	else
		if (theOldWeight < inNewWeight)
			LGraph_ForAllNodes(This->mGraph, theNode)
			{
				CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
				CDSSSP_IncreaseEdge(theSPTree, inEdgeUV, inNewWeight - theOldWeight);
			}
}

/* ---------------------------------------------------------------------------------
*  DeleteEdge
*  ---------------------------------------------------------------------------------
*  Side Effect inEdgeCost, inGraph deletes the edge in all CDSSSP */
void CDAPSP_DE_DeleteEdge(CDAPSP_DE* This, LGraph_TEdge* inEdgeUV)
{
	LGraph_TNode* theNode;
    LGraph_TEdge theCopy;
	LGraph_TEdge* thePtrCopy;

	theCopy.mSource = inEdgeUV->mSource;
	theCopy.mTarget = inEdgeUV->mTarget;
	thePtrCopy = &theCopy;

	LGraph_ForAllNodes(This->mGraph, theNode)
	{
		CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
		CDSSSP_DeleteEdge(theSPTree, thePtrCopy);
	}
}

/* ---------------------------------------------------------------------------------
*  InsertEdge
*  ---------------------------------------------------------------------------------
*  Side Effect inEdgeCost, inGraph inserts the edge in all CDSSSP */
void CDAPSP_DE_InsertEdge(CDAPSP_DE* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg, ui4 inWeight)
{
	LGraph_TNode* theNode;

	LGraph_NewEdge(This->mGraph, inSrc, inTrg);
	LGraph_ForAllNodes(This->mGraph, theNode)
	{
		CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
		CDSSSP_InsertEdge(theSPTree, inSrc, inTrg, inWeight );
	}
}

/* ---------------------------------------------------------------------------------
*  GetDistance
*  ---------------------------------------------------------------------------------
*  returns the distance between two nodes*/
ui4 CDAPSP_DE_GetDistance(CDAPSP_DE* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg)
{
	CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, inSrc);

	return CDSSSP_GetNodeDistance(theSPTree, inTrg);
}

/* ---------------------------------------------------------------------------------
*  GetUsedMem
*  ---------------------------------------------------------------------------------
*  returns the memory usage  */
ui4 CDAPSP_DE_GetUsedMem(CDAPSP_DE* This)
{
	LGraph_TNode* theNode;

	ui4 theMem = 0;

	theMem += sizeof(CDAPSP_DE);
	theMem += LNodeInfo_GetUsedMem(This->mSSSP);
	LGraph_ForAllNodes(This->mGraph, theNode)
	{
		CDSSSP* theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This->mSSSP, theNode);
		theMem += CDSSSP_GetUsedMem(theSPTree);
	}
	theMem += LHeap_GetUsedMem(This->mHeap);
	theMem += LArray_GetUsedMem(This->mArray0);
	theMem += LArray_GetUsedMem(This->mArray1);
	theMem += LArray_GetUsedMem(This->mGraphNodes);
	return theMem;
}

/* ---------------------------------------------------------------------------------
*  _Dealloc
*  ---------------------------------------------------------------------------------
*  Destructor for CDSSSP */
void _DeallocSSSP(LNodeInfo* This, LGraph_TNode* inNode)
{
	CDSSSP* theSPTree;

	theSPTree = (CDSSSP*)LNodeInfo_PointerAt(This, inNode);
	if (theSPTree)
		CDSSSP_Delete(&theSPTree);
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

