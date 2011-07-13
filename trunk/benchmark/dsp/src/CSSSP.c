/* ============================================================================
 *  CSSSP.c
 * ============================================================================

 *  Author:         (c) 2003 Camil Demetrescu, Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        January 9, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:06:04 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "CSSSP.h"
#include "_LGraph.h"
#include "LHeap.h"
#include "LException.h"
#include "LArray.h"
#include "LMemory.h"

#define CSSSP_INFINITY LType_MAX_UI4

/* Comparator for the Heap */
static Bool _Comp(ui4 inA, ui4 inB) {
    return inA < inB;
}

/* ---------------------------------------------------------------------------------
 *  CSSSP_UI4
 * ---------------------------------------------------------------------------------
 * Computes classic Dijkstra SSSP algorithm with UI4 weight on edges */
void CSSSP_UI4(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inWeightArray,
                     LNodeInfo** outDistArray, LNodeInfo** outPredArray) 
{

    LGraph_TNode* theNode;
    LGraph_TEdge* theEdge;
    LNodeInfo*    thePQIdx = NULL;
    LHeap*        thePQ    = NULL;
    ui4           theDist  = 0;

    /* edge weights must have type ui4 */
    if (!LType_EqualTypes(LEdgeInfo_GetBaseType(inWeightArray), LType_UI4)) return;

    Try {

        /* init priority queue and index map */
        thePQIdx = LNodeInfo_New(inGraph, LType_UI4);
        thePQ    = LHeap_New(_Comp);

        /* init output info */
        LGraph_ForAllNodes(inGraph, theNode) {
			LNodeInfo_UI4At(*outDistArray, theNode) = CSSSP_INFINITY;
            if (outPredArray!=NULL) 
				LNodeInfo_PointerAt(*outPredArray, theNode) = NULL;
        }

		/* enqueue source node */
        LNodeInfo_UI4At(thePQIdx, inSource) = 
            LHeap_Add(thePQ, (void*)inSource, theDist);
		LNodeInfo_UI4At(*outDistArray, inSource) = 0;

        /* main loop */
        while (!LHeap_Empty(thePQ)) {
    
            /* extract node with minimum distance from inSource */
            LHeap_ExtractMin(thePQ, (void**)&theNode, &theDist);

            /* scan adjacents */
            LGraph_ForAllOut(theNode, theEdge) {

                LGraph_TNode* theAdjNode    = LGraph_GetTarget(theEdge);
                ui4           theAdjDist    = theDist + LEdgeInfo_UI4At(inWeightArray, theEdge);

				if (LEdgeInfo_UI4At(inWeightArray, theEdge) == CSSSP_INFINITY)
					theAdjDist = CSSSP_INFINITY;

				/* if the graph is undirected, we have to check that the AdjNode is correct */
				if ((!inGraph->mDirected) && (theAdjNode == theNode))
					theAdjNode = LGraph_GetSource(theEdge);

                /* relaxation */
                if (theAdjDist < LNodeInfo_UI4At(*outDistArray, theAdjNode)) {

                    /* insert or update node key in priority queue */
                    if (LNodeInfo_UI4At(*outDistArray, theAdjNode) == CSSSP_INFINITY) 
                         LNodeInfo_UI4At(thePQIdx, theAdjNode) = 
                             LHeap_Add(thePQ, (void*)theAdjNode, theAdjDist);
                    else 
						LHeap_Update(thePQ, (void*)theAdjNode, theAdjDist, 
									 LNodeInfo_UI4At(thePQIdx, theAdjNode));

                    /* update distance and (possibly) precedessor */
                    LNodeInfo_UI4At(*outDistArray, theAdjNode) = theAdjDist;
                    if (outPredArray!=NULL) 
						LNodeInfo_PointerAt(*outPredArray, theAdjNode) = (void*)theNode;
                }
            }
        }
    }

    /* cleanup */
    CatchAny {
        if (thePQIdx != NULL) LNodeInfo_Delete(&thePQIdx);
        if (thePQ != NULL)    LHeap_Delete(&thePQ);
        Rethrow;
    }

    LNodeInfo_Delete(&thePQIdx);
    LHeap_Delete(&thePQ);
}

/* Copyright (C) 2003 Camil Demetrescu, Stefano Emiliozzi

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
