/* ============================================================================
 *  LDataStore.h
 * ============================================================================

 *  Author:         (c) 2001 Irene Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:03 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDataStore__
#define __LDataStore__

#include "LType.h"
#include "LArray.h"
#include "LXPBlock.h"
#include "LException.h"

/* COMPONENT ID */
#define LDataStore_ID   0x8003

/* EXCEPTION CODES */
enum {
    LDataStore_ILLEGAL_BLOCK_TYPE = LDataStore_ID << 16,
    LDataStore_NEWER_BLOCK_VERSION
};

#define LDataStore_MAGIC_NUMBER            0x4453544F      /* 'DSTO' */
#define LDataStore_VERSION                 0x0000

typedef ui4 LDataStore_TIdx;

typedef struct LDataStore LDataStore;

LDataStore*         LDataStore_New              ();
LDataStore*         LDataStore_NewFromData      (LXPBlock* thruBlock);
void                LDataStore_Delete           (LDataStore** ThisA);

LDataStore_TIdx     LDataStore_AddBlock         (LDataStore* This, const void* inItem, ui4 inSize);
LDataStore_TIdx     LDataStore_AddXPBlock       (LDataStore* This, const LXPBlock* inItem);
LDataStore_TIdx     LDataStore_AddUniqueBlock   (LDataStore* This, const void* inItem, ui4 inSize);
Bool                LDataStore_GetBlockIndex    (LDataStore* This, const void* inItem, ui4 inSize, LDataStore_TIdx* outIndex);
ui4                 LDataStore_GetOffsetByIndex (LDataStore* This, LDataStore_TIdx inIndex);
ui4                 LDataStore_GetSizeByIndex   (LDataStore* This, LDataStore_TIdx inIndex);
void                LDataStore_FetchBlockByIndex(LDataStore* This, LDataStore_TIdx inIndex, void* outItem);
void*               LDataStore_GetDataByIndex   (LDataStore* This, LDataStore_TIdx inIndex);
LXPBlock*           LDataStore_GetXPBlockByIndex(LDataStore* This, LDataStore_TIdx inIndex);
void*               LDataStore_GetData          (LDataStore* This);
ui4                 LDataStore_GetSize          (LDataStore* This);
void*               LDataStore_GetMap           (LDataStore* This);
ui4                 LDataStore_GetMapSize       (LDataStore* This);
ui4                 LDataStore_GetBlocksCount   (LDataStore* This);
LXPBlock*           LDataStore_GetXPBlock       (LDataStore* This);

#endif


/* Copyright (C) 2001 Irene Finocchi

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
