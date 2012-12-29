/* ============================================================================
 *  LDStar.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 2, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:48 $  
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LDStar.h"
#include "LGraph.h"
#include "LNodeInfo.h"
#include "LEdgeInfo.h"
#include "LGraphGen.h"
#include "LHash.h"
#include "LEdgeMap.h"
#include "LMemory.h"
#include "LHeap.h"
#include <time.h>

#define NO_VAL   LType_MAX_UI4
#define NO_PATH  0

typedef struct TSlot TSlot;

struct TSlot
{
	ui4			  mValue;
	LGraph_TNode* mState;
};

struct LDStar
{
	LGraph_TNode*	mGoal;
	LGraph_TNode*	mStart;
	LGraph_TNode*	mCurrent;
	LGraph*			mMap;
	LArray*			mNodesList;
	LEdgeInfo*		mCost;
	LNodeInfo*		mTag;
	LNodeInfo*		mBackPtr;
	LNodeInfo*		mH;
	LNodeInfo*		mNumberIterations;
	LHash*			mKIndices;
	//TSlot*			mK;
	LHeap*			mK;
	LGraph_TNode*	mMinState;
	LEdgeMap*		mEdgeMap;
	LDStar_TPoint*  mProcessedNodes;
	ui4				mOpenListCount;
	ui4				mPathCost;
	ui4				mBase;
	Bool			mFinished;
	ui4				mTotal_processed_nodes;
	ui4				mNodes_processed_since_last;
	Bool			mDebug;
};

/* Private methods declaration */
ui4             K_Insert(LDStar*, ui4, LGraph_TNode*);
LGraph_TNode*   K_GetState(LDStar*);
ui4				K_GetMin(LDStar*);
void			K_Init(LDStar*);
LGraph_TNode*	MinState(LDStar*);
void			Insert(LDStar*, LGraph_TNode*, ui4);
void			Delete(LDStar*, LGraph_TNode*);
ui4				GetKMin(LDStar*);
ui4				ProcessState(LDStar*);
ui4				ModifyCost(LDStar*, LGraph_TNode*, LGraph_TNode*, ui4);
/* Comparator for the LHeap object */
static Bool		_Comp(ui4 inA, ui4 inB); 

/* Utility Macros */
#define MIN(a, b) ((a<=b)? a : b)

/* Typecast Macros */
#define NODE *(LGraph_TNode**)
#define UI4  *(ui4*)

/* **************************** PUBLIC METHODS ********************************** */

/* ---------------------------------------------------------------------------------
*  New
*  ---------------------------------------------------------------------------------
*  Constructor */
LDStar*	LDStar_New(ui4 inN, LDStar_TPoint inStart, LDStar_TPoint inGoal)
{
	LDStar theObject;
	LGraph_TNode* theNode;

	theObject.mBase				= inN;
	theObject.mMap				= LGraphGen_Grid((ui2)inN, FALSE);
	theObject.mNodesList		= LGraph_GetAllNodes(theObject.mMap);
    theObject.mCost				= LGraphGen_RndEdgeInfoUI4(theObject.mMap, 0, 0, (i4)time(NULL));
	theObject.mTag				= LNodeInfo_New(theObject.mMap, LType_UI1);
	theObject.mBackPtr			= LNodeInfo_New(theObject.mMap, LType_Ptr);
	theObject.mH				= LNodeInfo_New(theObject.mMap, LType_UI4);
	theObject.mNumberIterations = LNodeInfo_New(theObject.mMap, LType_UI4);
	theObject.mK			    = LHeap_New(_Comp);
	theObject.mKIndices         = LHash_New();
	theObject.mEdgeMap          = LEdgeMap_New(theObject.mMap);
	theObject.mProcessedNodes   = (LDStar_TPoint*)LMemory_Malloc(inN*inN*sizeof(LDStar_TPoint));
	theObject.mStart			= NODE LArray_ItemAt(theObject.mNodesList, inStart.mX*inN+inStart.mY);
	theObject.mCurrent          = theObject.mStart;
	theObject.mGoal				= NODE LArray_ItemAt(theObject.mNodesList, inGoal.mX*inN+inGoal.mY);
	theObject.mPathCost         = 0;
	theObject.mFinished         = FALSE;
	theObject.mMinState			= NULL;
	theObject.mTotal_processed_nodes      = 0;
	theObject.mNodes_processed_since_last = 0;
	theObject.mOpenListCount              = 0;
	theObject.mDebug					  = FALSE;
	LGraph_ForAllNodes(theObject.mMap, theNode)
	{
		LNodeInfo_UI1At(theObject.mTag, theNode) = LDStar_NEW_NODE;
		LNodeInfo_UI4At(theObject.mH, theNode) = 0;
		NODE LNodeInfo_ItemAt(theObject.mBackPtr, theNode) = NULL;
	}
	return LMemory_NewObject(LDStar, theObject);
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void LDStar_Delete(LDStar** ThisA)
{
	LGraph_Delete(&((*ThisA)->mMap));
	LArray_Delete(&((*ThisA)->mNodesList));
	LEdgeInfo_Delete(&((*ThisA)->mCost));
	LNodeInfo_Delete(&((*ThisA)->mTag));
	LNodeInfo_Delete(&((*ThisA)->mBackPtr));
	LNodeInfo_Delete(&((*ThisA)->mH));
	LNodeInfo_Delete(&((*ThisA)->mNumberIterations));
	LHeap_Delete(&((*ThisA)->mK));
	LHash_Delete(&((*ThisA)->mKIndices));
	LEdgeMap_Delete(&((*ThisA)->mEdgeMap));
	LMemory_Free(&((*ThisA)->mProcessedNodes));
	LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
*  SetEdge
*  ---------------------------------------------------------------------------------
*  Sets the weight of the edge between (inP0, inP1) 
*  if the agent has already moved, it re-plans the shortest path */
void LDStar_SetEdge(LDStar* This, LDStar_TPoint inP0, LDStar_TPoint inP1, ui4 inValue)
{
	LGraph_TEdge* theEdge;
	LGraph_TNode* theX;
	LGraph_TNode* theY;
	ui4 theVal;
	ui4 theH_x;
	ui4 theH_y;
	Bool theModified = FALSE;

	if (This->mCurrent != This->mGoal)
	{
		theX = NODE LArray_ItemAt(This->mNodesList, inP0.mX*This->mBase+inP0.mY);
		theY = NODE LArray_ItemAt(This->mNodesList, inP1.mX*This->mBase+inP1.mY);
		theEdge = LEdgeMap_GetEdge(This->mEdgeMap, theX, theY);
		if (LEdgeInfo_UI4At(This->mCost, theEdge) != inValue)
		{
			theModified = TRUE;
			if (This->mCurrent != This->mStart)
				{
					theH_x = LNodeInfo_UI4At(This->mH, theX);
					theH_y = LNodeInfo_UI4At(This->mH, theY);

					if (theH_x < theH_y)
						theVal = ModifyCost(This, theX, theY, inValue);
					else
						theVal = ModifyCost(This, theY, theX, inValue);
				}
				else
					if (theEdge) LEdgeInfo_UI4At(This->mCost, theEdge) = inValue;
		}
		if ( (This->mCurrent != This->mStart) && (theModified) )
		{
			while ( (theVal < LNodeInfo_UI4At(This->mH, This->mCurrent)) && (theVal != NO_VAL) )
				theVal = ProcessState(This);
		}
	}
}

/* ---------------------------------------------------------------------------------
*  GetNodeInfo
*  ---------------------------------------------------------------------------------
*  Returns useful infos about nodes of the map */
LDStar_TNodeInfo LDStar_GetNodeInfo(LDStar* This, LDStar_TPoint inP)
{
	LGraph_TNode* theX;
	LGraph_TNode* theB;
	LGraph_TNode* theState = NULL;
	LDStar_TNodeInfo theInfo;
	ui4 theIdx;
	ui4 theK_x;

	theInfo.mPos.mX = inP.mX;
	theInfo.mPos.mY = inP.mY;
	theX = NODE LArray_ItemAt(This->mNodesList, inP.mX*This->mBase+inP.mY);
	theInfo.mIterations = LNodeInfo_UI4At(This->mNumberIterations, theX);
	theInfo.mH = LNodeInfo_UI4At(This->mH, theX);
	theInfo.mTag = LNodeInfo_UI1At(This->mTag, theX);
	if ( (theInfo.mTag) == LDStar_OPEN_NODE)
	{
		theIdx = (ui4)LHash_GetItemByKey(This->mKIndices, (ui4)theX->mIndex);
		LHeap_GetEntryAt(This->mK, (void**)&theState, (ui4*)&theK_x, theIdx);
		if ( theK_x < theInfo.mH)
			theInfo.mStatus = LDStar_LOWER_NODE;
		else
			if ( theK_x == theInfo.mH)
				theInfo.mStatus = LDStar_LOWER_NODE;
	}
	theB = NODE LNodeInfo_ItemAt(This->mBackPtr, theX);
	if (theB)
	{
		theInfo.mBptr.mX = theB->mIndex / This->mBase;
		theInfo.mBptr.mY = theB->mIndex % This->mBase;
	}
	return theInfo;
}

/* ---------------------------------------------------------------------------------
*  MoveAgent
*  ---------------------------------------------------------------------------------
*  Moves the agent by inN steps along the shortest path and  returns the list of   
*  passed states. It saves on outLen the length of the returned array. Note that the 
*  is returned only if inN > 1 otherwise NULL is returned. This is to avoid the user to
*  deallocate the returned array every time that he calls MoveAgent with 1 as inN. The 
*  returned array is useful when on large maps the user calls MoveAgent with large values
*  of inN and wants to know where the agent is passed */
LDStar_TPoint* LDStar_MoveAgent(LDStar* This, i4 inN, i4* outLen)
{
	ui4 theIdx;
	LDStar_TPoint* thePath = NULL;
	LGraph_TNode* theB_current;
	ui4 theVal = 0;
	LGraph_TEdge* theEdge;
	i4 i;
	
	if (This->mFinished) 
	{
		*outLen = 0;
		return NULL;
	}
	/* first path finding */
	if (This->mCurrent == This->mStart)
	{
		LGraph_TNode* theNode;
		LGraph_ForAllNodes(This->mMap, theNode)
		{	
			LNodeInfo_UI1At(This->mTag, theNode) = LDStar_NEW_NODE;
			LNodeInfo_UI4At(This->mH, theNode) = 0;
			LNodeInfo_UI4At(This->mNumberIterations, theNode) = 0;
			NODE LNodeInfo_ItemAt(This->mBackPtr, theNode) = NULL;
		}
		theIdx = LHeap_Add(This->mK, (void*)This->mGoal, 0);
		This->mOpenListCount++;
		LHash_InsertItem(This->mKIndices, (void*)theIdx, (ui4)This->mGoal->mIndex);
		while ((LNodeInfo_UI1At(This->mTag, This->mCurrent) != LDStar_CLOSED_NODE) 
				&& (theVal != NO_VAL))
			theVal = ProcessState(This);
		if (LNodeInfo_UI1At(This->mTag, This->mCurrent) == LDStar_NEW_NODE)
		{/* no path has been found */
			*outLen = 0;
			This->mFinished = TRUE;
			return NULL;
		}	
	}

	if ( inN < 0)
		inN = This->mBase*This->mBase;
	*outLen = inN;
	if (inN > 1) 
		thePath = (LDStar_TPoint*)LMemory_Malloc(inN*sizeof(LDStar_TPoint));
	for (i=0; i < inN; i++)
	{
		if (thePath)
		{
			thePath[i].mX = (This->mCurrent->mIndex) / This->mBase;
			thePath[i].mY = (This->mCurrent->mIndex) % This->mBase;
		}
		theB_current = NODE LNodeInfo_ItemAt(This->mBackPtr, This->mCurrent); 
		if (theB_current)
		{
			theEdge = LEdgeMap_GetEdge(This->mEdgeMap, This->mCurrent, theB_current);
			This->mPathCost += LEdgeInfo_UI4At(This->mCost, theEdge);
		}
		if (This->mCurrent == This->mGoal)
		{
			This->mFinished = TRUE;
			*outLen = i + 1;
			break;
		}
		This->mCurrent = theB_current;
	}
	return thePath;
}

/* ---------------------------------------------------------------------------------
*  CurrentLocation
*  ---------------------------------------------------------------------------------
*  Returns the current location of the agent */
LDStar_TPoint LDStar_CurrentLocation(LDStar* This)
{
	LDStar_TPoint theP;

	theP.mX = This->mCurrent->mIndex / This->mBase;
	theP.mY = This->mCurrent->mIndex % This->mBase;
	return theP;
}

/* ---------------------------------------------------------------------------------
*  GetEdgeCost
*  ---------------------------------------------------------------------------------
*  Returns the cost of the edge (inP0, inP1) of the map */
ui4 LDStar_GetEdgeCost(LDStar* This, LDStar_TPoint inP0, LDStar_TPoint inP1)
{
	LGraph_TNode* theX;
	LGraph_TNode* theY;
	LGraph_TEdge* theEdge;

	theX = NODE LArray_ItemAt(This->mNodesList, inP0.mX*This->mBase+inP0.mY);
	theY = NODE LArray_ItemAt(This->mNodesList, inP1.mX*This->mBase+inP1.mY);
	theEdge = LEdgeMap_GetEdge(This->mEdgeMap, theX, theY);
	if (theEdge) 
		return LEdgeInfo_UI4At(This->mCost, theEdge);
	else
		return 0;
}

/* ---------------------------------------------------------------------------------
*  GetStart
*  ---------------------------------------------------------------------------------
*  Returns the start point of the agent */
LDStar_TPoint LDStar_GetStart(LDStar* This)
{
	LDStar_TPoint theP;

	theP.mX = This->mStart->mIndex / This->mBase;
	theP.mY = This->mStart->mIndex % This->mBase;
	return theP;
}

/* ---------------------------------------------------------------------------------
*  GetGoal
*  ---------------------------------------------------------------------------------
*  Returns the goal point of the agent */
LDStar_TPoint LDStar_GetGoal(LDStar* This)
{
	LDStar_TPoint theP;

	theP.mX = This->mGoal->mIndex / This->mBase;
	theP.mY = This->mGoal->mIndex % This->mBase;
	return theP;
}

/* ---------------------------------------------------------------------------------
*  GetProcessedNodes
*  ---------------------------------------------------------------------------------
*  Returns a list of processed nodes since last re-planning */
LDStar_TPoint* LDStar_GetProcessedNodes(LDStar* This)
{ return This->mProcessedNodes; }

/* ---------------------------------------------------------------------------------
*  Finished
*  ---------------------------------------------------------------------------------
*  Returns TRUE if the agent is at goal or no path exists */
Bool LDStar_Finished(LDStar* This)
{ return This->mFinished; }

/* ---------------------------------------------------------------------------------
*  TotalProcessedNodes
*  ---------------------------------------------------------------------------------
*  Returns the number of total processed nodes since the beginning */
ui4	LDStar_TotalProcessedNodes(LDStar* This)
{ return This->mTotal_processed_nodes; }

/* ---------------------------------------------------------------------------------
*  NodesProcessedSinceLast
*  ---------------------------------------------------------------------------------
*  Returns the number of processed nodes since last re-plannig */
ui4	LDStar_NodesProcessedSinceLast(LDStar* This)
{ return This->mNodes_processed_since_last; }

/* ---------------------------------------------------------------------------------
*  GetBase
*  ---------------------------------------------------------------------------------
*  Returns the base side (n) of the nxn map */
ui4	LDStar_GetBase(LDStar* This)
{ return This->mBase; }

/* ---------------------------------------------------------------------------------
*  Dump
*  ---------------------------------------------------------------------------------
*  Draws on screen a text representation of the map */
void LDStar_Dump(LDStar* This)
{
	ui4 i;
	ui4 j;
	LGraph_TNode* p;
	LGraph_TNode* q;
	LGraph_TEdge* theEdge;

	if (!This->mDebug) return;
	for (i=0; i < This->mBase; i++)
	{
		for (j=0; j < This->mBase; j++)
		{
			p = NODE LArray_ItemAt(This->mNodesList, i*This->mBase+j);
			LDebug_Print("[%p]", p);
            if (j < This->mBase-1)
			{
				q = NODE LArray_ItemAt(This->mNodesList, i*This->mBase+j+1);
				theEdge = LEdgeMap_GetEdge(This->mEdgeMap, p, q);
				if (LEdgeInfo_UI4At(This->mCost, theEdge) == 1)
					LDebug_Print("-");
				else
					LDebug_Print(" ");
			}
		}
		LDebug_Print("\n");
		for (j=0; j < This->mBase; j++)
		{
			p = NODE LArray_ItemAt(This->mNodesList, i*This->mBase+j);
            if (i < This->mBase-1)
			{
				q = NODE LArray_ItemAt(This->mNodesList, (i+1)*This->mBase+j);
				theEdge = LEdgeMap_GetEdge(This->mEdgeMap, p, q);
				if (LEdgeInfo_UI4At(This->mCost, theEdge) == 1)
					LDebug_Print("    |      ");
				else
					LDebug_Print("           ");
			}
		}
		LDebug_Print("\n");
	}
}

/* ---------------------------------------------------------------------------------
*  SetDebug
*  ---------------------------------------------------------------------------------
*  Sets the debug or not */
void LDStar_SetDebug(LDStar* This, Bool inDebug)
{ This->mDebug = inDebug; }


/* **************************** PRIVATE METHODS ********************************** */
/* ---------------------------------------------------------------------------------
*  ModifyCost
*  ---------------------------------------------------------------------------------
*  Modifies the cost of an edge and inserts affected nodes in the Open List */
ui4 ModifyCost(LDStar* This, LGraph_TNode* inX, LGraph_TNode* inY, ui4 inValue)
{
	ui4            theH_x = LNodeInfo_UI4At(This->mH, inX);
    LGraph_TEdge* theEdge = LEdgeMap_GetEdge(This->mEdgeMap, inX, inY);


	LEdgeInfo_UI4At(This->mCost, theEdge) = inValue;
	if (LNodeInfo_UI1At(This->mTag, inX) == LDStar_CLOSED_NODE)
		Insert(This, inX, theH_x);
	This->mNodes_processed_since_last = 0;
	return GetKMin(This);
}

/* ---------------------------------------------------------------------------------
*  MinState
*  ---------------------------------------------------------------------------------
*  Extracts state with minimum key value from the K vector */
LGraph_TNode* MinState(LDStar* This)
{
	if (!This->mOpenListCount)
		return NULL;
	else
		return This->mMinState;
}

/* ---------------------------------------------------------------------------------
*  Insert
*  ---------------------------------------------------------------------------------
*  Inserts a new state in the Open List with a new H value */
void Insert(LDStar* This, LGraph_TNode* inX, ui4 inH_new)
{
	ui1	theTag_x;
	ui4	theK_x;
	ui4	theH_x;	
	ui4 theIdx;
	ui4 theMin;
	LGraph_TNode* theState = NULL;

    theTag_x = LNodeInfo_UI1At(This->mTag, inX);
	theH_x = LNodeInfo_UI4At(This->mH, inX);

    if (theTag_x == LDStar_NEW_NODE) 
	{
		theIdx = LHeap_Add(This->mK, (void*)inX, inH_new);
		This->mOpenListCount++;
		LHash_InsertItem(This->mKIndices, (void*)theIdx, (ui4)inX->mIndex);
	}
	else
	{
		if (theTag_x == LDStar_OPEN_NODE)
		{
			theIdx = (ui4)LHash_GetItemByKey(This->mKIndices, (ui4)inX->mIndex);
			LHeap_GetEntryAt(This->mK, (void**)&theState, (ui4*)&theK_x, theIdx);
			theMin = MIN(theK_x, inH_new);
			LHeap_Update(This->mK, (void*)theState, theMin, theIdx);
		}
		else
		{
			theH_x = LNodeInfo_UI4At(This->mH, inX);
			theMin = MIN(theH_x, inH_new);
			theIdx = LHeap_Add(This->mK, (void*)inX, theMin);
			This->mOpenListCount++;
			LHash_InsertItem(This->mKIndices, (void*)theIdx, (ui4)inX->mIndex);
		}
	}
	LNodeInfo_UI4At(This->mH, inX) = inH_new;
	LNodeInfo_UI1At(This->mTag, inX) = LDStar_OPEN_NODE;
	if (This->mDebug)
		LDebug_Print("Inserting node [%p] in the open list,  Tag[%p] = OPEN\n", inX, inX);
}

/* ---------------------------------------------------------------------------------
*  Delete
*  ---------------------------------------------------------------------------------
*  Deletes a state from the open list */
void Delete(LDStar* This, LGraph_TNode* inX)
{
	ui4 theIdx;

	/* we must update K(x) */
	theIdx = (ui4)LHash_GetItemByKey(This->mKIndices, (ui4)inX->mIndex);
	LHash_RemoveItem(This->mKIndices, (ui4)inX->mIndex);
	This->mOpenListCount--;
	LHeap_Remove(This->mK, theIdx);
	LNodeInfo_UI1At(This->mTag, inX) = LDStar_CLOSED_NODE;
	if (This->mDebug) 
		LDebug_Print("Deleting node [%p] from the open list, Tag[%p] = CLOSED\n", inX, inX);
}

/* ---------------------------------------------------------------------------------
*  K_GetMin
*  ---------------------------------------------------------------------------------
*  Extracts the minimum key value from the K vector */
ui4 GetKMin(LDStar* This)
{
	ui4 theMin;
	
	if (!This->mOpenListCount) 
		return NO_VAL;
	LHeap_GetMin(This->mK, (void**)&(This->mMinState), (ui4*)&theMin);
	return theMin;
}

/* ---------------------------------------------------------------------------------
*  ProcessState
*  ---------------------------------------------------------------------------------
*  This is the main D* function, don't mess with it!!!!!!! */
ui4 ProcessState(LDStar* This)
{
	ui4				theH_y;
	ui4				theH_x;
	ui4				theCost_xy;
	ui1				theTag_y;
	LGraph_TNode*	theB_y;
	ui4				theK_old;
	LGraph_TNode*	theX;
	LGraph_TNode*	theY;
	LGraph_TEdge*	theEdge;
	ui4				theIndex;
	
	theK_old = GetKMin(This);
    theX = MinState(This);
	if (theX) LNodeInfo_UI4At(This->mNumberIterations, theX)++;
	if (This->mDebug)
		LDebug_Print("Examining node [%p]\n", theX);
	if (theX == NULL) return NO_VAL;
	Delete(This, theX);
	if (theK_old < LNodeInfo_UI4At(This->mH, theX))
	{
		LGraph_ForAllOut(theX, theEdge)
		{
			theCost_xy = LEdgeInfo_UI4At(This->mCost, theEdge);
			theY = (LGraph_GetSource(theEdge) == theX) ? LGraph_GetTarget(theEdge):
														 LGraph_GetSource(theEdge);
			theH_y = LNodeInfo_UI4At(This->mH,theY);
			theH_x = LNodeInfo_UI4At(This->mH,theX);
			if ((theH_y <= theK_old) && (theH_x > theH_y + theCost_xy))
			{
				NODE LNodeInfo_ItemAt(This->mBackPtr, theX) = theY;
				if (This->mDebug)
					LDebug_Print("B(%p) = %p \n", theX, theY);
				LNodeInfo_UI4At(This->mH, theX) = theH_y + theCost_xy;
			}
		}
	}
	if (theK_old == LNodeInfo_UI4At(This->mH, theX))
	{
		LGraph_ForAllOut(theX, theEdge)
		{
			theCost_xy = LEdgeInfo_UI4At(This->mCost, theEdge);
			theY = (LGraph_GetSource(theEdge) == theX) ? LGraph_GetTarget(theEdge):
														 LGraph_GetSource(theEdge);
			theTag_y = LNodeInfo_UI1At(This->mTag, theY);
			theB_y = NODE LNodeInfo_ItemAt(This->mBackPtr, theY);
			theH_y = LNodeInfo_UI4At(This->mH,theY);
			theH_x = LNodeInfo_UI4At(This->mH,theX);
			if ((theTag_y == LDStar_NEW_NODE)							|| 
			    ( (theB_y == theX) && (theH_y != theH_x + theCost_xy) ) ||
			    ( (theB_y != theX) && (theH_y  > theH_x + theCost_xy) ))
			{
				NODE LNodeInfo_ItemAt(This->mBackPtr, theY) = theX;
				if (This->mDebug)
					LDebug_Print("B(%p) = %p \n", theY, theX);
				Insert(This, theY, theH_x + theCost_xy);
			}
		}
	}
	else
	{
		LGraph_ForAllOut(theX, theEdge)
		{
			theCost_xy = LEdgeInfo_UI4At(This->mCost, theEdge);
			theY = (LGraph_GetSource(theEdge) == theX) ? LGraph_GetTarget(theEdge):
									      				 LGraph_GetSource(theEdge);
			theTag_y = LNodeInfo_UI1At(This->mTag, theY);
			theB_y = NODE LNodeInfo_ItemAt(This->mBackPtr, theY);
			theH_y = LNodeInfo_UI4At(This->mH,theY);
			theH_x = LNodeInfo_UI4At(This->mH,theX);
			if ((theTag_y == LDStar_NEW_NODE)				||
				( (theB_y == theX) && (theH_y != theH_x + theCost_xy) )) 
			{
				NODE LNodeInfo_ItemAt(This->mBackPtr, theY) = theX;
				if (This->mDebug)
					LDebug_Print("B(%p) = %p \n", theY, theX);
				Insert(This, theY, theH_x + theCost_xy);
			}
			else
				if ( (theB_y != theX) && (theH_y > theH_x + theCost_xy) )
					Insert(This, theX, theH_x);
				else
					if ( (theB_y != theX) && (theH_x > theH_y + theCost_xy) &&
						 (theTag_y == LDStar_CLOSED_NODE) && (theH_y > theK_old) )
						Insert(This, theY, theH_y);
		}
	}
	if (This->mCurrent != This->mStart)
	{
		theIndex = This->mNodes_processed_since_last;
		This->mProcessedNodes[theIndex].mX = theX->mIndex / This->mBase;
		This->mProcessedNodes[theIndex].mY = theX->mIndex % This->mBase;
		This->mNodes_processed_since_last++;
	}
	
	This->mTotal_processed_nodes++;
	return GetKMin(This);
}

static Bool _Comp(ui4 inA, ui4 inB) 
{
    return inA < inB;
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
