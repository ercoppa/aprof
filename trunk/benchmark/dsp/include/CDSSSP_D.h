/* ============================================================================
 *  CDSSSP_D.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 17, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:51 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __CDSSSP_D__
#define __CDSSSP_D__

#include "LType.h"
#include "LGraph.h"
#include "LNodeInfo.h"
#include "LHash.h"
#include "LEdgeInfo.h"
#include "LEdgeMap.h"
#include "CDAPSP_D.h"
#include "CSSSP.h"

#define CDSSSP_D_ID 0x8032

#define CDSSSP_D_GetNodeDistance(Object, Node) (LNodeInfo_UI4At(Object->mL, Node))

typedef struct CDSSSP_D CDSSSP_D;

/* Object Struct */
struct CDSSSP_D
{
	LArray*	      mQ;			  /* main queue 					   */
	LNodeInfo*    mL;			  /* l(w) values					   */
	LNodeInfo*    mUnsettled;     /* settle status                     */
	LEdgeInfo*    mEdgeCost;      /* cost of the edges of the graph    */
	LEdgeMap*     mGraphMap;      /* used to track edges in the Graph  */
	LGraph*	      mGraph;         /* input graph					   */
	LArray*       mGraphNodes;    /* nodes of the graph				   */
	LGraph*	      mSPTree;		  /* internal SPTree				   */
	LArray*       mTreeNodes;     /* nodes of the tree				   */
	LGraph_TNode* mSource;		  /* source node of the SP Tree        */
	ui4           mDValue;		  /* max D distance					   */
	CDAPSP_D*     mCDAPSP_D;
	void          (*mCallBack)(CDAPSP_D*, LGraph_TNode*);
};

CDSSSP_D* CDSSSP_D_New(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inEdgeCost,
					   LArray* inGraphNodes, LArray* inArray, ui4 inD);

void          CDSSSP_D_Delete         (CDSSSP_D** ThisA);
void		  CDSSSP_D_AddEdge        (CDSSSP_D* This, LGraph_TEdge* inEdgeUV);
void		  CDSSSP_D_IncreaseEdge   (CDSSSP_D* This, LGraph_TEdge* inEdgeUV, ui4 inDelta);
void		  CDSSSP_D_DeleteEdge     (CDSSSP_D* This, LGraph_TEdge* inEdgeUV);
//ui4           CDSSSP_D_GetNodeDistance(CDSSSP_D* This, LGraph_TNode* inNode);
LGraph_TNode* CDSSSP_D_GetNodeParent  (CDSSSP_D* This, LGraph_TNode* inNode);
LGraph_TNode* CDSSSP_D_GetSourceNode  (CDSSSP_D* This);
ui4           CDSSSP_D_GetUsedMem     (CDSSSP_D* This);
void          CDSSSP_D_InstallCallBack(CDSSSP_D* This, CDAPSP_D* inCDAPSP_D,
									   void (*inCallBack)(CDAPSP_D*, LGraph_TNode*));
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

