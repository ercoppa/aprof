/* ============================================================================
 *  LDStar.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 2, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:03 $  
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDStar__
#define __LDStar__

/* INCLUDES */
#include "LType.h"

/* DEFINES */
#define LDStar_ID			0x8031
#define LDStar_NEW_NODE     0
#define LDStar_OPEN_NODE	10
#define	LDStar_CLOSED_NODE  20
#define	LDStar_LOWER_NODE	30
#define LDStar_RAISE_NODE	40

/* TYPEDEFS */
typedef struct LDStar LDStar;
typedef struct LDStar_TPoint LDStar_TPoint;
typedef struct LDStar_TNodeInfo LDStar_TNodeInfo;

struct LDStar_TPoint
{/* this represents points in the map, [(0,0) is upper-left corner] */
	ui4 mX;
	ui4 mY;
};

struct LDStar_TNodeInfo
{	
	LDStar_TPoint	mPos;		 /* (row,col) in the map [(0,0) is upper-left corner]  */
	LDStar_TPoint	mBptr;		 /* BackPointer of the node							   */
	ui4				mH;			 /* estimated distance from goal					   */	
	ui2				mTag;		 /* new/open/closed									   */
	ui2				mStatus;	 /* raise/lower NOTE: it is valid only for open states */
	ui4				mIterations; /* number of iteration on the node					   */	
};

/* PUBLIC FUNCTIONS */
LDStar*				LDStar_New						(ui4 inN, LDStar_TPoint inStart, 
													 LDStar_TPoint inGoal);
void				LDStar_Delete					(LDStar** ThisA);
void				LDStar_SetEdge					(LDStar* This, LDStar_TPoint inP0, 
													 LDStar_TPoint inP1, ui4 inValue);
LDStar_TNodeInfo	LDStar_GetNodeInfo				(LDStar* This, LDStar_TPoint inP);
LDStar_TPoint*		LDStar_MoveAgent				(LDStar* This, i4 inN, i4* outLen);
ui4					LDStar_GetCost					(LDStar* This);
LDStar_TPoint		LDStar_CurrentLocation			(LDStar* This);
LDStar_TPoint		LDStar_GetStart					(LDStar* This);
LDStar_TPoint		LDStar_GetGoal					(LDStar* This);
ui4					LDStar_GetEdgeCost				(LDStar* This, LDStar_TPoint inP0, 
													 LDStar_TPoint inP1);
LDStar_TPoint*		LDStar_GetProcessedNodes		(LDStar* This);
Bool				LDStar_Finished					(LDStar* This);
ui4					LDStar_TotalProcessedNodes		(LDStar* This);
ui4					LDStar_NodesProcessedSinceLast	(LDStar* This);
ui4					LDStar_GetBase					(LDStar* This);

void			    LDStar_Dump						(LDStar* This);
void			    LDStar_SetDebug					(LDStar* This, Bool inDebug);

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
