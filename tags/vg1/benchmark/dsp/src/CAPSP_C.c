/* ============================================================================
 *  CAPSP_C.c
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 21, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:00 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/


#include "CAPSP_C.h"
#include "CSSSP.h"
#include "_LGraph.h"
#include "LNodeInfo.h"
#include "LMemory.h"


/* ---------------------------------------------------------------------------------
 *  APSP_UI4
 * ---------------------------------------------------------------------------------
 * Computes classic APSP algorithm with UI4 weight on edges */
ui4* CAPSP_C_UI4(LGraph* inGraph, LEdgeInfo* inWeightArray)
{
	LNodeInfo**    theNodeInfoList = NULL;
	ui4		       i, j, N;
	LArray*		   theNodesList;
	LGraph_TNode** theNodes;
	ui4* 		   theDist = NULL;
	

	if ((!inGraph) || (!inWeightArray))
		return theDist;
	N = inGraph->mNodesCount;
	theNodeInfoList = (LNodeInfo**)LMemory_Malloc(N*sizeof(LNodeInfo*));
	theNodesList = LGraph_GetAllNodes(inGraph);
	LArray_InstallSyncPtr(theNodesList, (void**)&theNodes);
	theDist = (ui4*)LMemory_Malloc(N*N*sizeof(ui4));
    for (i = 0; i < N; i++)
	{
		theNodeInfoList[i] = LNodeInfo_New(inGraph, LType_UI4);
		CSSSP_UI4(inGraph, theNodes[i], inWeightArray, &theNodeInfoList[i], NULL);
		for (j = 0; j < N; j++)
			theDist[i*N+j] = LNodeInfo_UI4At(theNodeInfoList[i], theNodes[j]);
		LNodeInfo_Delete(&theNodeInfoList[i]);
	}
	/* Cleanup */
	LMemory_Free(&theNodeInfoList);
	LArray_Delete(&theNodesList);
	return theDist;
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

