/* ============================================================================
 *  LSP.h
 * ============================================================================

 *  Author:         (c) 2001-2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 4, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:28 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LSP__
#define __LSP__

#include "LType.h"
#include "LHeap.h"
#include "LGraph.h"
#include "LEdgeInfo.h"

/* COMPONENT ID */
#define LSP_ID    0x8025

/* DEFINES */
#define LSP_INFTY        LType_MAX_UI4
#ifndef LSP_STATISTICS
#define LSP_STATISTICS   1   /* To disable statistics collecting, set to 0 */
#endif

/* TYPEDEFS */
typedef struct {
    ui4 mW;			/* could be removed: mDist is sufficient... */
    ui4 mDist;
    ui4 mTB;        /* tie breaking tag */
    ui4 mHeapIdx;
    ui2 mL;
    ui2 mR;
    ui2 mFirstL;
    ui2 mFirstR;
    ui2 mNextL;
    ui2 mNextR;
} _LSP_TMatrixEntry; /* 24 bytes */

typedef struct {
    _LSP_TMatrixEntry*  mMatrixData;
    _LSP_TMatrixEntry** mMatrix;
    ui4                 mN;
    ui4                 mM;
    LHeap*              mHeap;

    #if LSP_STATISTICS == 1
    ui4                 mUP;
    ui4                 mHeapDecr;
    #endif
} LSP;

/* EXCEPTION CODES */
enum {
    LSP_BAD_PARAMETERS = LSP_ID<<16
};

/* PUBLIC FUNCTION PROTOTYPES */
LSP*    LSP_New            (LGraph* inGraph, LEdgeInfo* inWeightArray);
void    LSP_Delete         (LSP** AThis);

#define LSP_GetDist(This,x,y) ((This)->mMatrix[LGraph_GetNodeIndex(x)][LGraph_GetNodeIndex(y)].mDist)
#define LSP_GetLWit(This,x,y) ((This)->mMatrix[LGraph_GetNodeIndex(x)][LGraph_GetNodeIndex(y)].mL)
#define LSP_GetRWit(This,x,y) ((This)->mMatrix[LGraph_GetNodeIndex(x)][LGraph_GetNodeIndex(y)].mR)

ui4     LSP_GetUsedMem     (LSP* This);
void    LSP_Dump           (LSP* This);

#endif


/* Copyright (C) 2001-2003 Camil Demetrescu

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
