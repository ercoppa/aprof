/* ============================================================================
 *  LDSP.h
 * ============================================================================

 *  Author:         (c) 2002-2003 Camil Demetrescu. All rights reserved.
 *  License:        See the end of this file for license information
 *  Created:        October 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:02 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDSP__
#define __LDSP__

#include "LType.h"
#include "LGraph.h"
#include "LEdgeInfo.h"

/* COMPONENT ID */
#define LDSP_ID    0x8027

/* DEFINES */
#define LDSP_INFTY   LType_MAX_UI4
#define LDSP_NIL     0xFFFF

/* TYPEDEFS */
typedef struct LDSP LDSP;

typedef struct {       /* INFO RETURNED BY LDSP_GetStatistics */
    ui4 mPUP;          /* # potentially uniform paths */
    ui4 mUP;           /* # uniform paths */
    ui4 mHSP;          /* # historical shortest paths (shortest paths or zombies) */
    ui4 mSP;           /* # shortest paths */
    ui4 mZombie;       /* # zombies */
    ui4 mFree;         /* # free path slots in the data structure */
    ui4 mAffected;     /* # node pairs with distance changed by the last update */
    ui4 mNewPUP;       /* # potentially uniform paths created during the last update */
    ui4 mDelPUP;       /* # potentially uniform paths deleted during the last update */
    ui4 mDummyUpdates; /* total # dummy updates performed */
    ui4 mSkipped;      /* total # dummy updates skipped by adaptive strategy */
} LDSP_TStat;

typedef struct {
    f8  mThreshold;
    f8  mCorrection;
    ui4 mStep;
    ui4 mRatio;
} LDSP_TSetup;

/* EXCEPTION CODES */
enum {
    LDSP_BAD_PARAMETERS = LDSP_ID<<16,
    LDSP_INTERNAL_ERROR
};

/* PUBLIC FUNCTION PROTOTYPES */
LDSP*        LDSP_New            (LGraph* inGraph, LEdgeInfo* inEdgeWeights);
LDSP*        LDSP_NewEmpty       (ui2 inNumVertices);
void         LDSP_Delete         (LDSP** AThis);

ui2          LDSP_GetNumVertices (LDSP* This);
ui4          LDSP_GetEdgeWeight  (LDSP* This, ui2 inU, ui2 inV);

void         LDSP_UpdateEdge     (LDSP* This, ui2 inU, ui2 inV, ui4 inW);
ui4          LDSP_GetDist        (LDSP* This, ui2 inX, ui2 inY);

ui2          LDSP_GetLWit        (LDSP* This, ui2 inX, ui2 inY);
ui2          LDSP_GetRWit        (LDSP* This, ui2 inX, ui2 inY);

LDSP_TSetup  LDSP_GetConfig      (LDSP* This);
void         LDSP_SetConfig      (LDSP* This, LDSP_TSetup inSetup);
ui4          LDSP_GetUsedMem     (LDSP* This);
LDSP_TStat   LDSP_GetStatistics  (LDSP* This);
void         LDSP_Dump           (LDSP* This);

#endif

/* Copyright (C) 2002-2003 Camil Demetrescu 

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
