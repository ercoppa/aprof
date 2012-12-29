/* ============================================================================
 *  CDAPSP_D.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 23, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:50 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __CDAPSP_D__
#define __CDAPSP_D__

#include "LType.h"
#include "LGraph.h"
#include "LEdgeInfo.h"
#include "LNodeInfo.h"
#include "_LGraph.h"
#include "LHash.h"

#define CDAPSP_D_ID 0x8033

typedef struct CDAPSP_D CDAPSP_D;

/* Object Struct */
struct CDAPSP_D
{
	ui1			   mTreeMod;     /* used to see whether InV or OutV have been modified */
	LGraph_TNode*  mCurrNode;	 /* currently examined node during In Out update       */
	ui4            mDValue;		 /* max D distance					                   */
	ui4*           mDist;		 /* n x n distance matrix							   */
	LGraph*	       mGraph;       /* input graph					                       */
	LGraph*	       mRGraph;      /* input graph reversed    		                   */
	LNodeInfo*     mInV;		 /* InV for each vertex			                       */
	LNodeInfo*     mOutV;		 /* OutV for each vertex			                   */
	LEdgeInfo*     mEdgeCost;	 /* edges cost						                   */
    LEdgeInfo*     mREdgeCost;	 /* reversed graph edges cost		                   */
	LEdgeMap*      mRGraphMap;   /* used to traslate graph edges                       */
	LGraph_TNode** mList;        /* n x n x d matrix                                   */
	LArray*        mGraphNodes;  /* list of graph nodes					               */
	LArray*        mRGraphNodes; /* list of reversed graph nodes                       */
	LArray*        mArray;		 /* Array needed by CDSSSP_D				           */	
	LArray*        mInMod;       /* modified nodes in updated Inv                      */
	LArray*        mOutMod;      /* modified nodes in updated Outv                     */
	LHash*         mInIdx;
	LHash*         mOutIdx;
	ui4            mDelta;       /* increment value                                    */
	ui4            mOldWeight;   /* weight of the edge before the increment            */
};

#define CDAPSP_D_GetDistance(This, inX, inY) \
(This->mDist[inX->mIndex*This->mGraph->mNodesCount+inY->mIndex])

CDAPSP_D*     CDAPSP_D_New        (LGraph* inGraph, LEdgeInfo* inEdgeCost, ui4 inD);
void          CDAPSP_D_Delete     (CDAPSP_D** ThisA);
void          CDAPSP_D_Increase   (CDAPSP_D* This, LGraph_TEdge* inEdge, ui4 inDelta);
void          CDAPSP_D_Decrease   (CDAPSP_D* This, LGraph_TEdge* inEdge, ui4 inDelta);
ui4           CDAPSP_D_GetUsedMem (CDAPSP_D* This);
//ui4			  CDAPSP_D_GetDistance(CDAPSP_D* This, LGraph_TNode* inX, LGraph_TNode* inY); 
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

