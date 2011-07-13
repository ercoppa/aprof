/* ============================================================================
 *  LEdgeMap.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 24, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:50 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/


#include "_LEdgeMap.h"
#include "LHash.h"
#include "LArray.h"
#include "LNodeInfo.h"
#include "LException.h"
#include "LMemory.h"

/* Private methods declarations */

void _Sync(LEdgeMap* This, LGraph* inGraph);
void _Alloc(LNodeInfo* inNodeInfo, LGraph_TNode* inNode);
void _Dealloc(LNodeInfo* inNodeInfo, LGraph_TNode* inNode);


/* ---------------------------------------------------------------------------------
*  LEdgeInfo_New
*  ---------------------------------------------------------------------------------
*  Constructor */
LEdgeMap* LEdgeMap_New(LGraph* inGraph)
{
	LEdgeMap theObject = {0};
	LEdgeMap* theEdgeMap;
		
	Try
	{
		theObject.mNodeInfo  = LNodeInfo_New(inGraph, LType_Ptr);
		theObject.mGraph     = inGraph;
	}
	CatchAny
	{
		if (theObject.mNodeInfo)  LNodeInfo_Delete(&(theObject.mNodeInfo));
		Rethrow;
	}

	theEdgeMap = LMemory_NewObject(LEdgeMap, theObject);
	_Sync(theEdgeMap, inGraph);
	theEdgeMap->mGraphIdx = _LGraph_RegisterEdgeMap(inGraph, theEdgeMap);
	LNodeInfo_InstallDelNodeHandler(theEdgeMap->mNodeInfo, _Dealloc);
	LNodeInfo_InstallNewNodeHandler(theEdgeMap->mNodeInfo, _Alloc);
	return theEdgeMap;
}


/* ---------------------------------------------------------------------------------
*  LEdgeMap_Delete
*  ---------------------------------------------------------------------------------
*  Destructor */
void LEdgeMap_Delete(LEdgeMap** ThisA)
{
	if (!ThisA)    Throw(LEdgeMap_OBJECT_NULL_POINTER);
	if (!(*ThisA)) Throw(LEdgeMap_OBJECT_NULL_POINTER);
	
	if ( (*ThisA)->mGraph)
		_LGraph_UnregisterEdgeMap( (*ThisA)->mGraph, (*ThisA)->mGraphIdx);

	LNodeInfo_Delete(&((*ThisA)->mNodeInfo));
	LMemory_DeleteObject(ThisA);
}


/* ---------------------------------------------------------------------------------
*  LEdgeMap_EdgeExist
*  ---------------------------------------------------------------------------------
*  Returns TRUE <-> there's an edge from inSrc to inDst */
Bool LEdgeMap_EdgeExists(LEdgeMap* This, LGraph_TNode* inSrc, LGraph_TNode* inDst)
{
	LHash* theHash;

	if (!This) Throw(LEdgeMap_OBJECT_NULL_POINTER);
	if ( (!inSrc) || (!inDst) ) Throw(LEdgeMap_NODE_NULL_POINTER);

	if (!This->mGraph->mDirected)
	{/* if the graph is undirected a double check is necessary */
		LHash *theSrcHash, *theDstHash;

		theSrcHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inSrc);
		theDstHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inDst);
		return (LHash_IsInTable(theDstHash, (ui4)inSrc->mIndex) ||
				LHash_IsInTable(theSrcHash, (ui4)inDst->mIndex));
	
	}
	theHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inSrc);
	return LHash_IsInTable(theHash, (ui4)inDst->mIndex);
}

/* ---------------------------------------------------------------------------------
*  LEdgeMap_GetEdge
*  ---------------------------------------------------------------------------------
*  Returns the edge from inSrc to inDst, NULL if there's no edge */
LGraph_TEdge* LEdgeMap_GetEdge(LEdgeMap* This, LGraph_TNode* inSrc, LGraph_TNode* inDst)
{
	LHash* theHash;
	LGraph_TEdge* theEdge = NULL;

	if (!This) Throw(LEdgeMap_OBJECT_NULL_POINTER);
	if ( (!inSrc) || (!inDst) ) Throw(LEdgeMap_NODE_NULL_POINTER);

	if (!This->mGraph->mDirected)
	{/* if the graph is undirected a double check is necessary */
		LHash *theSrcHash, *theDstHash;

		theSrcHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inSrc);
		theDstHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inDst);
		if (LHash_IsInTable(theSrcHash, (ui4)inDst->mIndex))
			theEdge = (LGraph_TEdge*)LHash_GetItemByKey(theSrcHash, (ui4)inDst->mIndex);
		else
			if (LHash_IsInTable(theDstHash, (ui4)inSrc->mIndex))
				theEdge = (LGraph_TEdge*)LHash_GetItemByKey(theDstHash, (ui4)inSrc->mIndex);			
		return theEdge;
	}
	theHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inSrc);
	if (LHash_IsInTable(theHash, (ui4)inDst->mIndex))
		theEdge = (LGraph_TEdge*)LHash_GetItemByKey(theHash, (ui4)inDst->mIndex);
	return theEdge;
}

/* ---------------------------------------------------------------------------------
*  GetUsedMem
*  ---------------------------------------------------------------------------------
*  Returns the total memory usage of the object */
ui4 LEdgeMap_GetUsedMem (LEdgeMap* This)
{
	LGraph_TNode* theNode;
	ui4 theMemoryUsage = sizeof(LEdgeMap);

	theMemoryUsage += LNodeInfo_GetUsedMem(This->mNodeInfo);
	LGraph_ForAllNodes(This->mGraph, theNode)
	{
		LHash* theHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, theNode);
		theMemoryUsage += LHash_GetUsedMem(theHash);
	}
	return theMemoryUsage;
}

/* PRIVATE METHODS */
/* ---------------------------------------------------------------------------------
*  _Sync
*  ---------------------------------------------------------------------------------
*  Syncronizes the LEdgeMap with the bounded Graph */
void _Sync(LEdgeMap* This, LGraph* inGraph)
{
	LHash* theHash;
	ui4 theNodesCount;
	ui4 theAdjNodesCnt;
	LArray* theNodesList;
	LArray* theAdjList;
	LGraph_TNode* theNode;
	LGraph_TNode* theAdjNode;
	LGraph_TEdge* theEdge;
	ui4 i;
	ui4 j;

	theNodesCount = LGraph_GetNodesCount(This->mGraph);
	theNodesList  = LGraph_GetAllNodes(This->mGraph);

	for (i=0; i < theNodesCount; i++)
	{
		theNode = *(LGraph_TNode**) LArray_ItemAt(theNodesList, i);
		theAdjList = LGraph_GetOutEdges(inGraph, theNode);
		theAdjNodesCnt = LArray_GetItemsCount(theAdjList);
		theHash = LHash_New();
		for (j=0; j < theAdjNodesCnt; j++)
		{
			theEdge = *(LGraph_TEdge**) LArray_ItemAt(theAdjList, j);
			theAdjNode = (theNode == theEdge->mSource)? theEdge->mTarget : theEdge->mSource;
			LHash_InsertItem(theHash, (void*)theEdge, (ui4)theAdjNode->mIndex);
		}
		LNodeInfo_AssignItemAt(This->mNodeInfo, theNode, &theHash);
		LArray_Delete(&theAdjList);
	}
	LArray_Delete(&theNodesList);
}	

/* ---------------------------------------------------------------------------------
*  _LEdgeMapAddEdge
*  ---------------------------------------------------------------------------------
*  Adds an edge from the LEdgeMap */
void _LEdgeMap_AddEdge(LEdgeMap* This, LGraph_TEdge* inEdge)
{
	LHash* theHash;

	theHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inEdge->mSource);
	LHash_InsertItem(theHash, (void*)inEdge, (ui4)(inEdge->mTarget->mIndex));
}

/* ---------------------------------------------------------------------------------
*  _LEdgeMapDeleteEdge
*  ---------------------------------------------------------------------------------
*  Removes an edge from the LEdgeMap */
void _LEdgeMap_DeleteEdge(LEdgeMap* This, LGraph_TEdge* inEdge)
{
	LHash* theHash;

	theHash = *(LHash**) LNodeInfo_ItemAt(This->mNodeInfo, inEdge->mSource);
	LHash_RemoveItem(theHash, (ui4)(inEdge->mTarget->mIndex));
}

/* ---------------------------------------------------------------------------------
*  UnregisterGraph
*  ---------------------------------------------------------------------------------
*  Unregisters the Graph from the LEdgeMap */
void _LEdgeMap_UnregisterGraph(LEdgeMap* This)
{ This->mGraph = NULL; }

/* ---------------------------------------------------------------------------------
*  _Alloc
*  ---------------------------------------------------------------------------------
*  Handler to properly allocate the data structure used in LNodeInfo */
void _Alloc(LNodeInfo* inNodeInfo, LGraph_TNode* inNode)
{
	LHash* theHash = LHash_New();
	LNodeInfo_AssignItemAt(inNodeInfo, inNode, &theHash);
}

/* ---------------------------------------------------------------------------------
*  _Dealloc
*  ---------------------------------------------------------------------------------
*  Handler to properly deallocate the data structure used in LNodeInfo */
void _Dealloc(LNodeInfo* inNodeInfo, LGraph_TNode* inNode)
{
	LHash* theHash = *(LHash**) LNodeInfo_ItemAt(inNodeInfo, inNode);
	LHash_Delete(&theHash);
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

