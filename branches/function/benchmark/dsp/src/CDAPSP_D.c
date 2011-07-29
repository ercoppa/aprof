/* ============================================================================
 *  CDAPSP_D.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 23, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:01 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#include "CDAPSP_D.h"
#include "_CDSSSP_D.h"
#include "LEdgeMap.h"
#include "LHash.h"
#include "LMemory.h"
#include "LException.h"
#include "LGraphGen.h"
#include "_LGraph.h"

#define CDAPSP_D_MIN(a, b) (a < b)? a : b
#define CDAPSP_D_I(x,y,z) (x*(This->mDValue+1)*This->mGraph->mNodesCount+y*(This->mDValue+1)+z)
#define CDAPSP_D_INFINITY LType_MAX_UI4

/* Private methods declaration */
void  _BuildReversedEdgeCost(CDAPSP_D*);
void  _ComputeInOutTrees    (CDAPSP_D*);
void  _SelectBlockers       (CDAPSP_D*);
void  _BuildDistMatrix      (CDAPSP_D*);
void  _DecreaseUpdate	    (CDAPSP_D* , LGraph_TNode*);	
void  _IncreaseUpdate       (CDAPSP_D*);
void  _DeallocInOutV		(LNodeInfo*, LGraph_TNode*);
LGraph_TNode* _GetNewWitness(CDAPSP_D*, LGraph_TNode*, LGraph_TNode*, ui4, LGraph_TNode*);
void  _KeepModified         (CDAPSP_D*, LGraph_TNode*);

/* ---------------------------------------------------------------------------------
*  New
*  ---------------------------------------------------------------------------------
*  Constructor */
CDAPSP_D* CDAPSP_D_New(LGraph* inGraph, LEdgeInfo* inEdgeCost, ui4 inD)
{
	CDAPSP_D  theObject = {0};
	CDAPSP_D* This;
	ui4       N;

	/* check if parameters are OK */
	if ((!inGraph) || (!inEdgeCost))
		return NULL;
	N = inGraph->mNodesCount;
	theObject.mDValue = inD;
	Try
	{
		theObject.mGraph       = inGraph;
		theObject.mRGraph      = LGraphGen_ReverseGraph(inGraph);
		theObject.mREdgeCost   = LEdgeInfo_New(theObject.mRGraph, LType_UI4);
		theObject.mInV         = LNodeInfo_New(theObject.mRGraph, LType_Ptr);
		theObject.mOutV        = LNodeInfo_New(inGraph, LType_Ptr);
		theObject.mGraphNodes  = LGraph_GetAllNodes(theObject.mGraph);
		theObject.mRGraphNodes = LGraph_GetAllNodes(theObject.mRGraph);
		theObject.mRGraphMap   = LEdgeMap_New(theObject.mRGraph);
		theObject.mArray       = LArray_New(sizeof(LGraph_TNode*));
		theObject.mList        = (LGraph_TNode**)LMemory_Calloc(N*N*(inD+1)*sizeof(LGraph_TNode*));
		theObject.mDist        = (ui4*)LMemory_Malloc(N*N*sizeof(ui4));
		theObject.mEdgeCost	   = inEdgeCost;
		theObject.mInMod       = LArray_New(sizeof(LGraph_TNode*));
		theObject.mOutMod      = LArray_New(sizeof(LGraph_TNode*));
		theObject.mInIdx       = LHash_New();
		theObject.mOutIdx      = LHash_New();
	}
	CatchAny
	{/* cleaup */
		if (theObject.mRGraph)      LGraph_Delete(&theObject.mRGraph);
		if (theObject.mREdgeCost)   LEdgeInfo_Delete(&theObject.mREdgeCost);
		if (theObject.mInV)         LNodeInfo_Delete(&theObject.mInV);
		if (theObject.mOutV)        LNodeInfo_Delete(&theObject.mOutV);
		if (theObject.mGraphNodes)  LArray_Delete(&theObject.mGraphNodes);
		if (theObject.mRGraphNodes) LArray_Delete(&theObject.mRGraphNodes);
		if (theObject.mArray)       LArray_Delete(&theObject.mArray);
		if (theObject.mList)        LMemory_Free(&theObject.mList);
		if (theObject.mInMod)       LArray_Delete(&theObject.mInMod);
		if (theObject.mOutMod)      LArray_Delete(&theObject.mOutMod);
        Rethrow;
	}
    This = LMemory_NewObject(CDAPSP_D, theObject);
	LNodeInfo_InstallDelNodeHandler(This->mInV,  _DeallocInOutV);
	LNodeInfo_InstallDelNodeHandler(This->mOutV, _DeallocInOutV);
	_BuildReversedEdgeCost(This);
	_ComputeInOutTrees(This);
	_BuildDistMatrix(This);
    return This;
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void CDAPSP_D_Delete(CDAPSP_D** ThisA)
{
	LMemory_Free(&(*ThisA)->mList);
	LMemory_Free(&(*ThisA)->mDist);
	LGraph_Delete(&(*ThisA)->mRGraph);
	LEdgeMap_Delete(&(*ThisA)->mRGraphMap);
	LEdgeInfo_Delete(&(*ThisA)->mREdgeCost);
	LNodeInfo_Delete(&(*ThisA)->mInV);
	LNodeInfo_Delete(&(*ThisA)->mOutV);
	LArray_Delete(&(*ThisA)->mInMod);
	LArray_Delete(&(*ThisA)->mOutMod);
    LArray_Delete(&(*ThisA)->mGraphNodes);
	LArray_Delete(&(*ThisA)->mRGraphNodes);
	LArray_Delete(&(*ThisA)->mArray);
	LHash_Delete(&(*ThisA)->mInIdx);
	LHash_Delete(&(*ThisA)->mOutIdx);
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  Increase
*  ---------------------------------------------------------------------------------
*  Increases the weight of inEdge and maintains the structure
*  NOTE: Since this object holds a pointer tho the weight LEdgeInfo, this function
*  assumes that the edge cost has been updated BEFORE its invocation */
void CDAPSP_D_Increase(CDAPSP_D* This, LGraph_TEdge* inEdge, ui4 inDelta)
{
	LGraph_TNode *theSrc, *theTrg;
	LGraph_TEdge* theREdge;

	This->mDelta = inDelta;
	/* reversed graph edge*/
	theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, inEdge->mTarget->mIndex);
	theTrg = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, inEdge->mSource->mIndex);
	theREdge = LEdgeMap_GetEdge(This->mRGraphMap, theSrc, theTrg);
	This->mOldWeight = LEdgeInfo_UI4At(This->mREdgeCost, theREdge);
	/* if inDelta is +oo we are performing a deletion */
	if (inDelta == CDAPSP_D_INFINITY)
		LEdgeInfo_UI4At(This->mREdgeCost, theREdge) = CDAPSP_D_INFINITY;
	else
		LEdgeInfo_UI4At(This->mREdgeCost, theREdge) += inDelta;

	LGraph_ForAllNodes(This->mRGraph, theSrc)
	{
		CDSSSP_D *theIn, *theOut;
		
		This->mTreeMod = 1;
		This->mCurrNode = theSrc;
		theIn = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV, theSrc);
		if (theIn)
			CDSSSP_D_IncreaseEdge(theIn, theREdge, inDelta);
		This->mTreeMod = 2;
		theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theSrc->mIndex);
		This->mCurrNode = theSrc;
		theOut = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theSrc);
		CDSSSP_D_IncreaseEdge(theOut, inEdge, inDelta);
	}
	_IncreaseUpdate(This);
}

/* ---------------------------------------------------------------------------------
*  Decrease
*  ---------------------------------------------------------------------------------
*  Decreases the weight of inEdge and maintains the structure
*  NOTE: Since this object holds a pointer tho the weight LEdgeInfo, this function
*  assumes that the edge cost has been updated BEFORE its invocation */
void CDAPSP_D_Decrease(CDAPSP_D* This, LGraph_TEdge* inEdge, ui4 inDelta)
{
	LGraph_TNode *theSrc, *theTrg;
	LGraph_TEdge* theREdge;
	CDSSSP_D *theIn, *theOut;
	
	/* reversed graph edge*/
	theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, inEdge->mTarget->mIndex);
	theTrg = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, inEdge->mSource->mIndex);
	theREdge = LEdgeMap_GetEdge(This->mRGraphMap, theSrc, theTrg);
	LEdgeInfo_UI4At(This->mREdgeCost, theREdge) -= inDelta;

	theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, inEdge->mSource->mIndex);
	/* gets InV */
	theIn = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV, theSrc);
	/* destroys InV */
	if (theIn)
		CDSSSP_D_Delete(&theIn);
	/* rebuilds InV */
	theIn = CDSSSP_D_New(This->mRGraph, theSrc, This->mREdgeCost, 
		This->mRGraphNodes, This->mArray, This->mDValue);

	CDSSSP_D_InstallCallBack(theIn, This, _KeepModified);
	LNodeInfo_PointerAt(This->mInV, theSrc) = theIn;

	theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, inEdge->mSource->mIndex);
	/* gets OutV */
	theOut = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theSrc);
	/* destroys OutV */
	CDSSSP_D_Delete(&theOut);
	/* rebuilds OutV */
	theOut = CDSSSP_D_New(This->mGraph, theSrc, This->mEdgeCost, 
		This->mGraphNodes, This->mArray, This->mDValue);
	CDSSSP_D_InstallCallBack(theOut, This, _KeepModified);
	LNodeInfo_PointerAt(This->mOutV, theSrc) = theOut;

	theSrc = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, inEdge->mSource->mIndex);
	_DecreaseUpdate(This, theSrc);
}

/* ---------------------------------------------------------------------------------
*  GetUsedMem
*  ---------------------------------------------------------------------------------
*  Returns total memory usage of the object */
ui4 CDAPSP_D_GetUsedMem(CDAPSP_D* This)
{
	ui4 theMem = 0;
	ui4 N = This->mGraph->mNodesCount;
	LGraph_TNode *theNode;

	theMem += N*N*(This->mDValue+1)*sizeof(LGraph_TNode*);
	theMem += N*N*sizeof(ui4);
	theMem += LGraph_GetUsedMem(This->mRGraph);
	theMem += LEdgeMap_GetUsedMem(This->mRGraphMap);
	theMem += LEdgeInfo_GetUsedMem(This->mREdgeCost);
	LGraph_ForAllNodes(This->mRGraph, theNode)
	{
		CDSSSP_D* theOutV;

		CDSSSP_D* theInV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV, theNode);
		if (theInV)
			theMem += CDSSSP_D_GetUsedMem(theInV);
		theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theNode);
		theMem += CDSSSP_D_GetUsedMem(theOutV);
	}
	theMem += LNodeInfo_GetUsedMem(This->mInV);
	theMem += LNodeInfo_GetUsedMem(This->mOutV);
	theMem += LArray_GetUsedMem(This->mGraphNodes);
	theMem += LArray_GetUsedMem(This->mArray);
	theMem += LArray_GetUsedMem(This->mRGraphNodes);
	theMem += LArray_GetUsedMem(This->mInMod);
	theMem += LArray_GetUsedMem(This->mOutMod);
	theMem += LHash_GetUsedMem(This->mInIdx);
	theMem += LHash_GetUsedMem(This->mOutIdx);
	theMem += sizeof(CDAPSP_D);
	return theMem;
}

/* Private Methods Definitions */

/* ---------------------------------------------------------------------------------
*  _BuildReversedEdgeCost
*  ---------------------------------------------------------------------------------
*  Assigns weights to the reversed edges LEdgeInfo */
void _BuildReversedEdgeCost(CDAPSP_D* This)
{
	LGraph_TEdge* theGraphEdge;
	LGraph_TEdge* theRGraphEdge;

	LGraph_ForAllEdges(This->mGraph, theGraphEdge)
	{
		LGraph_TNode *theGSrc, *theGTrg, *theRGSrc, *theRGTrg;
		ui4 theEdgeCost;

		theGSrc = theGraphEdge->mSource;
		theGTrg = theGraphEdge->mTarget;
		theRGSrc = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, theGSrc->mIndex);
		theRGTrg = *(LGraph_TNode**)LArray_ItemAt(This->mRGraphNodes, theGTrg->mIndex);
		theRGraphEdge = LEdgeMap_GetEdge(This->mRGraphMap, theRGTrg, theRGSrc);
		theEdgeCost = LEdgeInfo_UI4At(This->mEdgeCost, theGraphEdge);
		LEdgeInfo_UI4At(This->mREdgeCost, theRGraphEdge) = theEdgeCost;
	}
}

/* ---------------------------------------------------------------------------------
*  _ComputeInOutTrees
*  ---------------------------------------------------------------------------------
*  Creates initial In(v), Out(v) for all v */
void _ComputeInOutTrees(CDAPSP_D* This)
{
	CDSSSP_D     /* *theInV, */ *theOutV;
	LGraph_TNode *theRNode, *theNode;
	ui4 theSize = 0;

	LGraph_ForAllNodes(This->mRGraph, theRNode)
	{
		/* InTrees(v) *
		theInV = CDSSSP_D_New(This->mRGraph, theRNode, This->mREdgeCost, 
			This->mRGraphNodes, This->mArray, This->mDValue);*/

		LNodeInfo_PointerAt(This->mInV, theRNode) = NULL;//theInV;
		/*CDSSSP_D_InstallCallBack(theInV, This, _KeepModified);*/

		/* OutTrees(v) */
		theNode = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, theRNode->mIndex);
		theOutV = CDSSSP_D_New(This->mGraph, theNode, This->mEdgeCost, 
			This->mGraphNodes, This->mArray, This->mDValue);

		LNodeInfo_PointerAt(This->mOutV, theNode) = theOutV;
		CDSSSP_D_InstallCallBack(theOutV, This, _KeepModified);
		//theSize += CDSSSP_D_GetUsedMem(theInV);
		theSize += CDSSSP_D_GetUsedMem(theOutV);
	}
	//LDebug_Print("Trees size: %u\n", theSize);
}

/* ---------------------------------------------------------------------------------
*  _BuildDistMatrix
*  ---------------------------------------------------------------------------------
*  Builds Dist[u][v] for all u, v */
void _BuildDistMatrix(CDAPSP_D* This)
{
	ui4 i,j;
	ui4 N = This->mGraph->mNodesCount;
		
	for (i=0; i < N; i++)
	{
		LGraph_TNode* theV =*(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, i);
		CDSSSP_D* theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theV);
        for (j=0; j < N; j++)
		{
			LGraph_TNode* theW = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, j);
			ui4 theKeyW = CDSSSP_D_GetNodeDistance(theOutV, theW);
		
			This->mDist[i*N+j] = CDAPSP_D_INFINITY;
			if (theKeyW < CDAPSP_D_INFINITY)
			{
				This->mList[CDAPSP_D_I(i, j, theKeyW)] = theV;
				This->mDist[i*N+j] = theKeyW;
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  _IncreaseUpdate
*  ---------------------------------------------------------------------------------
*  Updates Dist[u][v] when increase has occurred */
void _IncreaseUpdate(CDAPSP_D* This)
{
	ui4 i,j,k;
	ui4      N = This->mGraph->mNodesCount;
	ui4      D = This->mDValue;
	ui4 theCnt = LArray_GetItemsCount(This->mInMod);
	
	/* inTrees updates */
	for(k = 0; k < theCnt; k++)
	{
		LGraph_TNode *theV, *theW;
		LGraph_TNode* theU = *(LGraph_TNode**)LArray_ItemAt(This->mInMod, 0);
		
		LArray_RemoveItemAt(This->mInMod, 0);
		LHash_RemoveItem(This->mInIdx, (ui4)theU->mIndex);
		for (j=0; j < N; j++)
		{
			theW = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, j);
			i = This->mDist[theU->mIndex*N+theW->mIndex];
			/* retrives old witness */
			theV = This->mList[CDAPSP_D_I(theU->mIndex, theW->mIndex, i)];
			if (theV)
			{
				CDSSSP_D* theInV  = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV,  theV);
				CDSSSP_D* theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theV);
				ui4 theKeyW       = CDSSSP_D_GetNodeDistance(theOutV, theW);
				ui4 theKeyU;

				if (theInV)
					theKeyU = CDSSSP_D_GetNodeDistance(theInV, theU);
				else
				{
					CDSSSP_D* theOutU = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theU);
					theKeyU = CDSSSP_D_GetNodeDistance(theOutU, theV);
				}
				if ((theKeyU < CDAPSP_D_INFINITY) && (theKeyW < CDAPSP_D_INFINITY))
				{
					if (theKeyU + theKeyW > i)
					{
						ui4 w;
						/* if the increment is Delta the node's distance cannot */
						/* increase more than Delta                             */
						ui4 theEnd     = CDAPSP_D_MIN(D, This->mDelta+i);
						ui4 theOldDist = This->mDist[theU->mIndex*N+theW->mIndex];
						LGraph_TNode* theWitness;

						/* corrects theEnd in case of edge deletion */
						if (This->mDelta == CDAPSP_D_INFINITY)
							theEnd = D;

						for (w=i; w <= theEnd; w++) 
						{
							theWitness = _GetNewWitness(This, theU, theW, w, theV);
							This->mList[CDAPSP_D_I(theU->mIndex, theW->mIndex, w)] = theWitness;
							if ((theWitness) && 
								(theOldDist == This->mDist[theU->mIndex*N+theW->mIndex]))
							{
								This->mDist[theU->mIndex*N+theW->mIndex] = w;
								break;
							}
						}
						if ((theOldDist == This->mDist[theU->mIndex*N+theW->mIndex]) &&
							(!theWitness))
							This->mDist[theU->mIndex*N+theW->mIndex] = CDAPSP_D_INFINITY;
					}
				}
				else
				{
					This->mDist[theU->mIndex*N+theW->mIndex] = CDAPSP_D_INFINITY;
					if (i < CDAPSP_D_INFINITY)
						This->mList[CDAPSP_D_I(theU->mIndex, theW->mIndex, i)] = NULL;
				}
			}
		}
	}
	/* OutTrees updates */
	theCnt = LArray_GetItemsCount(This->mOutMod);
	for(k = 0; k < theCnt; k++)
	{
		LGraph_TNode  *theV, *theW;
		LGraph_TNode* theU = *(LGraph_TNode**)LArray_ItemAt(This->mOutMod, 0);

		LArray_RemoveItemAt(This->mOutMod, 0);
		LHash_RemoveItem(This->mOutIdx, (ui4)theU->mIndex);
		for (j=0; j < N; j++)
		{
			theW = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, j);
			i=This->mDist[theW->mIndex*N+theU->mIndex];
			/* retrives old witness */
			theV = This->mList[CDAPSP_D_I(theW->mIndex, theU->mIndex, i)];
			if (theV)
			{
				CDSSSP_D* theInV  = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV,  theV);
				CDSSSP_D* theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theV);
				ui4 theKeyU       = CDSSSP_D_GetNodeDistance(theOutV, theU);
				ui4 theKeyW;

				if (theInV)
					theKeyW = CDSSSP_D_GetNodeDistance(theInV, theW);
				else
				{
					CDSSSP_D* theOutW = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theW);
					theKeyW = CDSSSP_D_GetNodeDistance(theOutW, theV);
				}
				if ((theKeyU < CDAPSP_D_INFINITY) && (theKeyW < CDAPSP_D_INFINITY))
				{
					if (theKeyU + theKeyW > i)
					{
						ui4 w;
						/* if the increment is Delta the node's distance cannot */
						/* increase more than Delta                             */
						ui4 theEnd = CDAPSP_D_MIN(D, This->mDelta+i);
						ui4 theOldDist = This->mDist[theW->mIndex*N+theU->mIndex];
						LGraph_TNode* theWitness;
						
						/* corrects theEnd in case of edge deletion */
						if (This->mDelta == CDAPSP_D_INFINITY)
							theEnd = D;

						for (w=i; w <= theEnd; w++) 
						{
							theWitness = _GetNewWitness(This, theW, theU, w, theV);
							This->mList[CDAPSP_D_I(theW->mIndex, theU->mIndex, w)] = theWitness;
							if ((theWitness) &&
								(theOldDist == This->mDist[theW->mIndex*N+theU->mIndex]))
							{
								This->mDist[theW->mIndex*N+theU->mIndex] = w;
								break;
							}
						}
						if ((theOldDist == This->mDist[theW->mIndex*N+theU->mIndex]) &&
							(!theWitness))
							This->mDist[theW->mIndex*N+theU->mIndex] = CDAPSP_D_INFINITY;
					}
				}
				else
				{
					This->mDist[theW->mIndex*N+theU->mIndex] = CDAPSP_D_INFINITY;
					if (i < CDAPSP_D_INFINITY)
						This->mList[CDAPSP_D_I(theW->mIndex, theU->mIndex, i)] = NULL;
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------------
*  _DecreaseUpdate
*  ---------------------------------------------------------------------------------
*  Updates Dist[u][v] when increase has occurred */
void _DecreaseUpdate(CDAPSP_D* This, LGraph_TNode* inNode)
{
	ui4 i,j;//,k;
	ui4 N = This->mGraph->mNodesCount;
	
	/* cleans inNode from all list in which it is a witness *
	for (i=0; i < N; i++)
		for (j=0; j < N; j++)
			for (k=0; k < This->mDValue; k++)
				if (This->mList[CDAPSP_D_I(i,j,k)] == inNode)
					This->mList[CDAPSP_D_I(i,j,k)] = NULL;*/
		
	for (i=0; i < N; i++)
		for (j=0; j < N; j++)
		{
			LGraph_TNode* theV = inNode;
			LGraph_TNode* theU = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, i);
			LGraph_TNode* theW = *(LGraph_TNode**)LArray_ItemAt(This->mGraphNodes, j);
			CDSSSP_D* theInV  = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV,  theV);
			CDSSSP_D* theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theV);
			ui4 theKeyU = CDSSSP_D_GetNodeDistance(theInV,  theU);
			ui4 theKeyW = CDSSSP_D_GetNodeDistance(theOutV, theW);
			
			if ((theKeyU < CDAPSP_D_INFINITY) && (theKeyW < CDAPSP_D_INFINITY))
			{
				ui4 theKey = theKeyU + theKeyW;
				
				/* Add v to the List(u,w,d) */
				if (theKey <= This->mDValue) 
				{
					if (!This->mList[CDAPSP_D_I(i, j, theKey)])
					{
						This->mList[CDAPSP_D_I(i, j, theKey)] = theV;
						if (theKey < This->mDist[i*N+j])
							This->mDist[i*N+j] = theKey;
					}
				}
			}
		}
}

/* ---------------------------------------------------------------------------------
*  _DeallocInOutV
*  ---------------------------------------------------------------------------------
*  Destructor for In(v), Out(v) */
void _DeallocInOutV(LNodeInfo* This, LGraph_TNode* inN)
{
	CDSSSP_D* theSPTree;

	theSPTree = (CDSSSP_D*)LNodeInfo_PointerAt(This, inN);
	if (theSPTree)
		CDSSSP_D_Delete(&theSPTree);
}

/* ---------------------------------------------------------------------------------
*  _GetNewWitness
*  ---------------------------------------------------------------------------------
*  Finds a new witness for the List(u,w,d) */
LGraph_TNode* _GetNewWitness(CDAPSP_D* This, LGraph_TNode* inU, LGraph_TNode* inW, ui4 inD, LGraph_TNode* inV)
{
	LGraph_TNode* theNode;

	LGraph_ForAllNodes(This->mGraph, theNode)
	{
		CDSSSP_D* theInV  = (CDSSSP_D*)LNodeInfo_PointerAt(This->mInV,  theNode);
		CDSSSP_D* theOutV = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, theNode);
		ui4 theKeyW       = CDSSSP_D_GetNodeDistance(theOutV, inW);
		ui4 theKeyU;

		if (theInV)
			theKeyU = CDSSSP_D_GetNodeDistance(theInV,  inU);
		else
		{
			CDSSSP_D* theOutU = (CDSSSP_D*)LNodeInfo_PointerAt(This->mOutV, inU);
			theKeyU = CDSSSP_D_GetNodeDistance(theOutU,  theNode);
		}
		if ((theKeyU < CDAPSP_D_INFINITY) && (theKeyW < CDAPSP_D_INFINITY))
			if (theKeyU + theKeyW == inD)
				return theNode;
	}
	return NULL;
}

/* ---------------------------------------------------------------------------------
*  _KeepModified
*  ---------------------------------------------------------------------------------
*  Saves the modified Nodes */
void  _KeepModified(CDAPSP_D* This, LGraph_TNode* inNode)
{
	if (This->mTreeMod == 1)
	{
		if (!LHash_IsInTable(This->mInIdx, (ui4)inNode->mIndex))
		{
			LArray_AppendItem(This->mInMod, (const void*)&inNode);
			LHash_InsertItem (This->mInIdx, NULL, (ui4)inNode->mIndex);
		}
	}
	else
	{
		if (!LHash_IsInTable(This->mOutIdx, (ui4)inNode->mIndex))
		{
			LArray_AppendItem(This->mOutMod, (const void*)&inNode);
			LHash_InsertItem (This->mOutIdx, NULL,(ui4)inNode->mIndex);
		}
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

