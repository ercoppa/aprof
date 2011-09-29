/* ============================================================================
 *  CDSSSP.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 17, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:51 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __CDSSSP__
#define __CDSSSP__

#include "LType.h"
#include "LGraph.h"
#include "LNodeInfo.h"
#include "LHeap.h"
#include "LHash.h"
#include "LEdgeInfo.h"
#include "LEdgeMap.h"
#include "CSSSP.h"

#define CDSSSP_ID 0x8037

typedef struct CDSSSP CDSSSP;

CDSSSP* CDSSSP_New      (LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost);
CDSSSP* CDSSSP_NewShared(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost,
						 LArray* inGraphNodes, LHeap* inHeap, LArray* inList0, LArray* inList1);

void          CDSSSP_Delete         (CDSSSP** ThisA);
void		  CDSSSP_UpdateEdge     (CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inNewWeight);
void		  CDSSSP_DeleteEdge     (CDSSSP* This, LGraph_TEdge* inEdgeUV);
void		  CDSSSP_InsertEdge     (CDSSSP* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg, ui4 inWeight);
ui4           CDSSSP_GetNodeDistance(CDSSSP* This, LGraph_TNode* inNode);
LGraph_TNode* CDSSSP_GetNodeParent  (CDSSSP* This, LGraph_TNode* inNode);
LGraph_TNode* CDSSSP_GetSourceNode  (CDSSSP* This);
ui4           CDSSSP_GetUsedMem     (CDSSSP* This);

#endif

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

