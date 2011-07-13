/* ============================================================================
 *  CDAPSP_DE.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        May 13, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:50 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __CDAPSP_DE__
#define __CDAPSP_DE__

#include "LType.h"
#include "LGraph.h"
#include "LNodeInfo.h"
#include "LHeap.h"
#include "LHash.h"
#include "LEdgeInfo.h"

#define CDAPSP_DE_ID 0x803A

typedef struct CDAPSP_DE CDAPSP_DE;

CDAPSP_DE* CDAPSP_DE_New          (LGraph* inGraph, LEdgeInfo* inEdgeCost);
void       CDAPSP_DE_Delete       (CDAPSP_DE** ThisA);
void	   CDAPSP_DE_UpdateEdge   (CDAPSP_DE* This, LGraph_TEdge* inEdgeUV, ui4 inNewWeight);
void	   CDAPSP_DE_DeleteEdge   (CDAPSP_DE* This, LGraph_TEdge* inEdgeUV);
void	   CDAPSP_DE_InsertEdge   (CDAPSP_DE* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg, ui4 inWeight);
ui4        CDAPSP_DE_GetDistance  (CDAPSP_DE* This, LGraph_TNode* inSrc, LGraph_TNode* inTrg);
ui4        CDAPSP_DE_GetUsedMem   (CDAPSP_DE* This);

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

