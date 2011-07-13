/* ============================================================================
 *  CDAPSP.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 23, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:00 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "CDAPSP.h"
#include "CAPSP_C.h"
#include "LMemory.h"
#include "LException.h"
#include "LMath.h"
#include "LRandSource.h"
#include "LGraphGen.h"
#include "_LGraph.h"
#include <time.h>
#include <stdio.h>

#define CDAPSP_MIN(a, b) (a < b)? a : b
#define CDAPSP_INFINITY LType_MAX_UI4

/* Private methods declaration */
void _SelectBlockers(CDAPSP*);
void _BuildGs       (CDAPSP*);
void _UpdateIncGs   (CDAPSP*);
void _UpdateDecGs   (CDAPSP*);
void _Stitch    	(CDAPSP*);
ui4  _GetMinDistXtoS(CDAPSP*, LGraph_TNode*, ui4);
ui4  _GetMinDistXtoY(CDAPSP*, LGraph_TNode*);
void CDAPSP_Increase(CDAPSP*, LGraph_TEdge*, ui4);
void CDAPSP_Decrease(CDAPSP*, LGraph_TEdge*, ui4);


/* ---------------------------------------------------------------------------------
*  New
*  ---------------------------------------------------------------------------------
*  Constructor */
CDAPSP* CDAPSP_New(LGraph* inGraph, LEdgeInfo* inEdgeCost, ui4 inC, f4 inAlpha, f4 inBeta)
{
	CDAPSP  theObject = {0};
	CDAPSP* This;
	ui4     N;
	f8      theFunction;

	/* check if parameters are OK */
	if ((!inGraph) || (!inEdgeCost) || (inC < 1))
		return NULL;
	N = inGraph->mNodesCount;
	
	theFunction = ((1-inAlpha)*LMath_Sqrt(N*inC*LMath_Log10(N)) + inAlpha*N*inC);
	theObject.mDValue = (ui4)(theFunction*inBeta);
	theObject.mSValue = (ui4)(N*inC*LMath_Log10(N)/theFunction*inBeta);
	if (theObject.mSValue > N)
		theObject.mSValue = N;
	Try
	{
		theObject.mGraph         = inGraph;
		theObject.mBlockGraph    = LGraph_New(TRUE);
		theObject.mBlockEdgeCost = LEdgeInfo_New(theObject.mBlockGraph, LType_UI4);
		theObject.mBlockList     = (LGraph_TNode**)LMemory_Malloc(theObject.mSValue*sizeof(LGraph_TNode*));
		theObject.mBGraphNodes   = (LGraph_TNode**)LMemory_Malloc(theObject.mSValue*sizeof(LGraph_TNode*));
		theObject.mGraphNodes    = LGraph_GetAllNodes(theObject.mGraph);
		theObject.mDistX_S       = (ui4*)LMemory_Malloc(theObject.mSValue*sizeof(ui4)); 
		theObject.mCDAPSP_D      = CDAPSP_D_New(inGraph, inEdgeCost, theObject.mDValue);
		theObject.mCValue        = inC;
		theObject.mEdgeCost      = inEdgeCost;
		theObject.mDist          = (ui4*)LMemory_Calloc(N*N*sizeof(ui4));
		This                     = LMemory_NewObject(CDAPSP, theObject);
		_SelectBlockers(This);
		_BuildGs(This);
		_Stitch(This);
	}
	CatchAny
	{/* cleaup */
		if (theObject.mBlockGraph)    LGraph_Delete(&theObject.mBlockGraph);
		if (theObject.mBlockEdgeCost) LEdgeInfo_Delete(&theObject.mBlockEdgeCost);
		if (theObject.mBlockList)     LMemory_Free(&theObject.mBlockList);
		if (theObject.mGraphNodes)    LArray_Delete(&theObject.mGraphNodes);
		if (theObject.mCDAPSP_D)      CDAPSP_D_Delete(&theObject.mCDAPSP_D);
		if (theObject.mDist)          LMemory_Free(&theObject.mDist);
		if (theObject.mDistX_S)       LMemory_Free(&theObject.mDistX_S);
		if (theObject.mBGraphNodes)   LMemory_Free(&theObject.mBGraphNodes);
		Rethrow;
	}
	return This;
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void CDAPSP_Delete(CDAPSP** ThisA)
{
	LGraph_Delete(&(*ThisA)->mBlockGraph);
	LEdgeInfo_Delete(&(*ThisA)->mBlockEdgeCost);
	LMemory_Free(&(*ThisA)->mBlockList);
	LArray_Delete(&(*ThisA)->mGraphNodes);
	LMemory_Free(&(*ThisA)->mBlockDist);
	LMemory_Free(&(*ThisA)->mDist);
	LMemory_Free(&(*ThisA)->mDistX_S);
	LMemory_Free(&(*ThisA)->mBGraphNodes);
	CDAPSP_D_Delete(&(*ThisA)->mCDAPSP_D);
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  Update
*  ---------------------------------------------------------------------------------
*  Updates the weight of inEdge and maintains the structure */
void CDAPSP_UpdateEdge(CDAPSP* This, LGraph_TEdge* inEdge, ui4 inNewWeight)
{
	ui4 theOldWeight = LEdgeInfo_UI4At(This->mEdgeCost, inEdge);
	
	if (inNewWeight == CDAPSP_INFINITY)
		CDAPSP_Increase(This, inEdge, inNewWeight);
	
	if (theOldWeight == inNewWeight) 
		return;
	
	if (theOldWeight > inNewWeight)
			CDAPSP_Decrease(This, inEdge, theOldWeight - inNewWeight);
	else
		if (theOldWeight < inNewWeight)
			CDAPSP_Increase(This, inEdge, inNewWeight - theOldWeight);

}

/* ---------------------------------------------------------------------------------
*  Increase
*  ---------------------------------------------------------------------------------
*  Increases the weight of inEdge and maintains the structure */
void CDAPSP_Increase(CDAPSP* This, LGraph_TEdge* inEdge, ui4 inDelta)
{
    ui4 theOldW = LEdgeInfo_UI4At(This->mEdgeCost, inEdge);

	if (inDelta < CDAPSP_INFINITY)
	{
		if ( theOldW + inDelta > This->mCValue )
			inDelta = This->mCValue - theOldW;

		if (inDelta == 0) return;
		LEdgeInfo_UI4At(This->mEdgeCost, inEdge) += inDelta;
		CDAPSP_D_Increase(This->mCDAPSP_D, inEdge, inDelta);
	}
	else
	{/* deletion case */
		LEdgeInfo_UI4At(This->mEdgeCost, inEdge) = inDelta;
        CDAPSP_D_Increase(This->mCDAPSP_D, inEdge, inDelta);
	}

    _UpdateIncGs(This);

	/* deallocates dist matrix */
	LMemory_Free(&This->mBlockDist);
	_Stitch(This);
}

/* ---------------------------------------------------------------------------------
*  Decrease
*  ---------------------------------------------------------------------------------
*  Decreases the weight of inEdge and maintains the structure
*  NOTE: Since this object holds a pointer to the weights LEdgeInfo, this function
*  assumes that the edge cost has been updated BEFORE its invocation */
void CDAPSP_Decrease(CDAPSP* This, LGraph_TEdge* inEdge, ui4 inDelta)
{
    ui4 theOldW = LEdgeInfo_UI4At(This->mEdgeCost, inEdge);

    if ( theOldW < inDelta ) inDelta = theOldW;
    if ( inDelta == 0 ) return;

    LEdgeInfo_UI4At(This->mEdgeCost, inEdge) -= inDelta;

	CDAPSP_D_Decrease(This->mCDAPSP_D, inEdge, inDelta);
	/* rebuilds blockers graph */
	_UpdateDecGs(This);
	/* deallocs dist matrix */
	LMemory_Free(&This->mBlockDist);
	_Stitch(This);
}

/* ---------------------------------------------------------------------------------
*  GetDistance
*  ---------------------------------------------------------------------------------
*  returns Dist(x, y) */
ui4 CDAPSP_GetDistance (CDAPSP* This, LGraph_TNode* inX, LGraph_TNode* inY)
{
	ui4 N = This->mGraph->mNodesCount;
	return This->mDist[inX->mIndex*N+inY->mIndex];
}

/* ---------------------------------------------------------------------------------
*  GetUsedMem
*  ---------------------------------------------------------------------------------
*  Returns total memory usage of the object */
ui4 CDAPSP_GetUsedMem(CDAPSP* This)
{
	ui4 theMem = 0;
	ui4 N = This->mGraph->mNodesCount;
	
	theMem += LGraph_GetUsedMem(This->mBlockGraph);
	theMem += LEdgeInfo_GetUsedMem(This->mBlockEdgeCost);
	theMem += 2*This->mSValue*sizeof(LGraph_TNode*);
	theMem += This->mSValue*sizeof(ui4);
	theMem += LArray_GetUsedMem(This->mGraphNodes);
	theMem += N*N*sizeof(ui4);
	theMem += This->mSValue*This->mSValue*sizeof(ui4);
	theMem += CDAPSP_D_GetUsedMem(This->mCDAPSP_D);
	theMem += sizeof(CDAPSP);
	return theMem;
}

/* ---------------------------------------------------------------------------------
*  CDAPSP_GetInfo
*  ---------------------------------------------------------------------------------
*  Returns useful infos about the data structures */
CDAPSP_TInfo CDAPSP_GetInfo(CDAPSP* This)
{
	CDAPSP_TInfo theInfo;

	theInfo.mBlockersCount = This->mSValue;
	theInfo.mBlockersGraphEdgesCount = This->mBlockGraph->mEdgesCount;
	theInfo.mD = This->mDValue;
	return theInfo;
}


/* PRIVATE METHODS */

/* ---------------------------------------------------------------------------------
*  SelectBlockers
*  ---------------------------------------------------------------------------------
*  Chooses |D| blockers at random */
void _SelectBlockers(CDAPSP* This)
{
	ui4 i;
	ui4 N = This->mGraph->mNodesCount;
	ui4* theIdxList = LMemory_Malloc( N * sizeof(ui4));
	LRandSource* theRand = LRandSource_New((ui4)time(NULL) );

	for (i=0; i < N; i++)
		theIdxList[i] = i;
	for (i=0; i < This->mSValue; i++)
	{
		ui4 theIdx = LRandSource_GetRandUI4(theRand, 0, N-1-i);
		This->mBlockList[i] = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theIdxList[theIdx]);
		theIdxList[theIdx] = theIdxList[N-1-i];
	}
	LRandSource_Delete(&theRand);
	LMemory_Free(&theIdxList);
}

/* ---------------------------------------------------------------------------------
*  BuildGs
*  ---------------------------------------------------------------------------------
*  Builds Gs graph */
void _BuildGs(CDAPSP* This)
{
	ui4 i,j;
	
	for (i=0; i < This->mSValue; i++)
		This->mBGraphNodes[i] = LGraph_NewNode(This->mBlockGraph);
	
	for (i=0; i < This->mSValue; i++)
	{
		/* gets i-th blockers index */ 
		ui4   theBlockerIdx = This->mBlockList[i]->mIndex;
		/* gets matching graph node */
		LGraph_TNode* theBlocker = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBlockerIdx);
		for (j=0; j < This->mSValue; j++)
		{
			ui4 theBlockIdx = This->mBlockList[j]->mIndex;
			LGraph_TNode* theBlock = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBlockIdx);
			if (theBlocker != theBlock)
			{
				ui4 thePathLength = 
					CDAPSP_D_GetDistance(This->mCDAPSP_D, theBlocker, theBlock);
				
				if (thePathLength < CDAPSP_INFINITY)
				{
					LGraph_TEdge* theEdge = LGraph_NewEdge(This->mBlockGraph, 
														   This->mBGraphNodes[i], 
												           This->mBGraphNodes[j]); 
					LEdgeInfo_UI4At(This->mBlockEdgeCost, theEdge) = thePathLength;
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  UpdateIncGs
*  ---------------------------------------------------------------------------------
*  Updates Gs graph after an Increase*/
void _UpdateIncGs(CDAPSP* This)
{
	LGraph_TEdge* theEdge;
	ui4 i;
	ui4 theCount;
	LArray* theBGraphEdges = LGraph_GetAllEdges(This->mBlockGraph);
	
	theCount = This->mBlockGraph->mEdgesCount;
	for (i=0; i < theCount; i++)
	{
		LGraph_TNode *theSrc, *theTrg, *theBSrc, *theBTrg, *theBGraphSrc, *theBGraphTrg;
		ui4 thePathLength;

		theEdge = *(LGraph_TEdge**)LArray_ItemAt(theBGraphEdges, i);
		theBGraphSrc = theEdge->mSource;
		theBGraphTrg = theEdge->mTarget;
		/* gets matching blockers */
		theBSrc = This->mBlockList[theBGraphSrc->mIndex];
		theBTrg = This->mBlockList[theBGraphTrg->mIndex];
		/* gets graph matching nodes */
		theSrc  = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBSrc->mIndex);
		theTrg  = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBTrg->mIndex);
		/* reads updated distance */
		thePathLength = CDAPSP_D_GetDistance(This->mCDAPSP_D, theSrc, theTrg);
		if (thePathLength < CDAPSP_INFINITY)/* update edge cost */
			LEdgeInfo_UI4At(This->mBlockEdgeCost, theEdge) = thePathLength;
		else
			/* removes the edge */
			LGraph_DelEdge(This->mBlockGraph, theEdge);
	}
	LArray_Delete(&theBGraphEdges);
}

/* ---------------------------------------------------------------------------------
*  UpdateDecGs
*  ---------------------------------------------------------------------------------
*  Updates Gs graph after a Decrease*/
void _UpdateDecGs(CDAPSP* This)
{
	ui4 i,j;
	LArray* theEdges = LGraph_GetAllEdges(This->mBlockGraph);
	ui4 theCnt = This->mBlockGraph->mEdgesCount;
	
	/* old edges cleanup */
	for (i=0; i < theCnt; i++)
		LGraph_DelEdge(This->mBlockGraph, *(LGraph_TEdge**)LArray_ItemAt(theEdges, i));
	LArray_Delete(&theEdges);
	for (i=0; i < This->mSValue; i++)
	{
		/* gets i-th blockers index */ 
		ui4   theBlockerIdx = This->mBlockList[i]->mIndex;
		/* gets matching graph node */
		LGraph_TNode* theBlocker0 = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBlockerIdx);
		for (j=0; j < This->mSValue; j++)
		{
			ui4 theBlockerIdx = This->mBlockList[j]->mIndex;
			LGraph_TNode* theBlocker1 = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theBlockerIdx);
			if (theBlocker0 != theBlocker1)
			{
				ui4 thePathLength = 
					CDAPSP_D_GetDistance(This->mCDAPSP_D, theBlocker0, theBlocker1);
				
				if (thePathLength < CDAPSP_INFINITY)
				{
					LGraph_TEdge* theEdge = LGraph_NewEdge(This->mBlockGraph, 
												   This->mBGraphNodes[i], 
					                               This->mBGraphNodes[j]); 
					LEdgeInfo_UI4At(This->mBlockEdgeCost, theEdge) = thePathLength;
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  Stitch
*  ---------------------------------------------------------------------------------
*  Applies the stitching algorithm */
void _Stitch(CDAPSP* This)
{
	LGraph_TNode   *theNode, *theTrgNode;
	ui4    N = This->mGraph->mNodesCount;
	/* Rebuild blockers dist matrix */
	This->mBlockDist = CAPSP_C_UI4(This->mBlockGraph, This->mBlockEdgeCost);
	LGraph_ForAllNodes(This->mGraph, theNode)
	{/* computes distances from theNode to all blockers */
		ui4 i;
		for (i=0; i < This->mSValue; i++)
		{
			LGraph_TNode* theBlocker = This->mBlockList[i];
			ui4 theDist_d_X_S = CDAPSP_D_GetDistance(This->mCDAPSP_D, theNode, theBlocker);

			This->mDistX_S[i] = CDAPSP_MIN(theDist_d_X_S, _GetMinDistXtoS(This, theNode, i));
		}
		LGraph_ForAllNodes(This->mGraph, theTrgNode)
		{
			if (theNode != theTrgNode)
			{
				ui4 i = theNode->mIndex;
				ui4 j = theTrgNode->mIndex;
				ui4 theDist_d_X_Y = CDAPSP_D_GetDistance(This->mCDAPSP_D, theNode, theTrgNode);
				This->mDist[i*N+j] =  
					CDAPSP_MIN(theDist_d_X_Y, _GetMinDistXtoY(This, theTrgNode));
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  GetMinDistXtoS
*  ---------------------------------------------------------------------------------
*  Returns min for_all s {dist_d_(x,s') + dist(s', s)} where s is a blocker */
ui4  _GetMinDistXtoS(CDAPSP* This, LGraph_TNode* inNode, ui4 inBlockIdx)
{
	ui4 i;
	ui4 theMin = CDAPSP_INFINITY;
	ui4    D = This->mSValue;
	
	for (i=0; i < This->mSValue; i++)
	{
		LGraph_TNode* theBlocker = This->mBlockList[i];
		ui4 theDist_d_X_S = CDAPSP_D_GetDistance(This->mCDAPSP_D, inNode, theBlocker);

		if ((theDist_d_X_S != CDAPSP_INFINITY) && (This->mBlockDist[i*D+inBlockIdx] != CDAPSP_INFINITY))
			theDist_d_X_S += This->mBlockDist[i*D+inBlockIdx];
		else
			theDist_d_X_S = CDAPSP_INFINITY;
		if (theDist_d_X_S < theMin)
			theMin = theDist_d_X_S;
	}
	return theMin;
}

/* ---------------------------------------------------------------------------------
*  GetMinDistXtoY
*  ---------------------------------------------------------------------------------
*  Returns min for_all s {dist_d_(x,s) + dist(s, y)} where s is a blocker */
ui4  _GetMinDistXtoY(CDAPSP* This, LGraph_TNode* inY)
{
	ui4 i;
	ui4 theMin = CDAPSP_INFINITY;
	
	for (i=0; i < This->mSValue; i++)
	{
		LGraph_TNode* theBlocker = This->mBlockList[i];
		ui4 theDist       = This->mDistX_S[i];
		ui4 theDist_d_S_Y = CDAPSP_D_GetDistance(This->mCDAPSP_D, theBlocker, inY);

		if ((theDist != CDAPSP_INFINITY) && (theDist_d_S_Y != CDAPSP_INFINITY))
			theDist += theDist_d_S_Y;
		else
			theDist = CDAPSP_INFINITY;
		if (theDist < theMin)
			theMin = theDist;
	}
	return theMin;
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

