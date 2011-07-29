/* ============================================================================
 *  CDAPSP.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 23, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:49 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef __CDAPSP__
#define __CDAPSP__

#include "LType.h"
#include "_LGraph.h"
#include "LEdgeInfo.h"
#include "CDAPSP_D.h"
#include "LSP.h"

#define CDAPSP_ID 0x8034

typedef struct CDAPSP CDAPSP;

/* Object Struct */
struct CDAPSP
{
	LGraph*	       mGraph;         /* input graph					   */
	LGraph*	       mBlockGraph;    /* blockers directed graph   	   */
	LEdgeInfo*     mEdgeCost;	   /* edges cost					   */
	LEdgeInfo*     mBlockEdgeCost; /* Gs graph edges cost			   */
	ui4            mDValue;		   /* max D distance				   */
	ui4            mCValue;		   /* max C distance				   */
	ui4            mSValue;        /* Number of blockers               */
	ui4*           mDist;		   /* integer dynamic distance matrix  */
	ui4*           mBlockDist;     /* distances matrix for Gs          */
	ui4*           mDistX_S;       /* distance from nodes to blockers  */ 
	LGraph_TNode** mBlockList;     /* blockers set                     */
	LArray*        mGraphNodes;    /* list of graph nodes              */
	LGraph_TNode** mBGraphNodes;   /* blockers graph nodes             */
	CDAPSP_D*      mCDAPSP_D;      /* used to query distances <= d     */
};

typedef struct CDAPSP_TInfo CDAPSP_TInfo;
struct CDAPSP_TInfo 
{
	ui4 mBlockersCount;
	ui4 mBlockersGraphEdgesCount;
	ui4 mD;
	f4  mAlpha;
	f4  mBeta;
};

typedef struct CDAPSP_TConfig CDAPSP_TConfig;
struct CDAPSP_TConfig 
{
	f4  mAlpha;
	f4  mBeta;
};



CDAPSP*      CDAPSP_New         (LGraph* inGraph, LEdgeInfo* inEdgeCost, ui4 inC, f4 inAlpha, f4 inBeta);
void         CDAPSP_Delete      (CDAPSP** ThisA);
void         CDAPSP_UpdateEdge  (CDAPSP* This, LGraph_TEdge* inEdge, ui4 inNewWeight);
ui4          CDAPSP_GetDistance (CDAPSP* This, LGraph_TNode* inX, LGraph_TNode* inY);
ui4          CDAPSP_GetUsedMem  (CDAPSP* This);
CDAPSP_TInfo CDAPSP_GetInfo     (CDAPSP* This);
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

