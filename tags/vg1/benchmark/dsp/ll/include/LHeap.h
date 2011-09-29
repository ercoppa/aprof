/* ============================================================================
 *  LHeap.h
 * ============================================================================

 *  Author:         (c) 2001-2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:14 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LHeap__
#define __LHeap__

#include "LType.h"

/* COMPONENT ID */
#define LHeap_ID   0x8026

/* TYPEDEFS */
typedef struct tagLHeap LHeap;
typedef Bool (*LHeap_TComparator)(ui4 inA, ui4 inB);

/* EXCEPTION CODES */
enum { 
    LHeap_BAD_INDEX = LHeap_ID<<16,
    LHeap_EMPTY_HEAP
};

/* PUBLIC FUNCTION PROTOTYPES */
LHeap*      LHeap_New        (LHeap_TComparator inComparator);
void        LHeap_Delete     (LHeap** ThisA);

Bool        LHeap_Empty      (LHeap* This);
ui4         LHeap_Add        (LHeap* This, const void* inItem, ui4 inKey);
void        LHeap_Remove     (LHeap* This, ui4 inIdx);
void        LHeap_Update     (LHeap* This, const void* inItem, ui4 inKey, ui4 inIdx);
void        LHeap_ExtractMin (LHeap* This, void** outItem, ui4* outKey);
void        LHeap_GetMin     (LHeap* This, void** outItem, ui4* outKey);
void        LHeap_GetEntryAt (LHeap* This, void** outItem, ui4* outKey, ui4 inIdx);

ui4         LHeap_GetUsedMem (LHeap* This);
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
