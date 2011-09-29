/* ============================================================================
 *  LXPBlock.h
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:43 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LXPBlock__
#define __LXPBlock__

#include "LType.h"

/* COMPONENT ID */
#define LXPBlock_ID 0x800A

typedef struct LXPBlock LXPBlock;

enum {
    LXPBlock_UNEXPECTED_END_OF_BLOCK = LXPBlock_ID<<16
};

typedef enum {
    LXPBlock_START      = 0x01,
    LXPBlock_CURR       = 0x02,
    LXPBlock_END        = 0x03
} LXPBlock_TSeekMode;


LXPBlock*           LXPBlock_New            ();
LXPBlock*           LXPBlock_NewFromData    (void** inDataA, ui4 inSize);
void                LXPBlock_Delete         (LXPBlock** ThisA);

void                LXPBlock_Write          (LXPBlock* This, const void* inData, ui4 inSize);
void                LXPBlock_Read           (LXPBlock* This, void* outData, ui4 inSize);

void                LXPBlock_WriteXPBlock   (LXPBlock* This, const LXPBlock* inXPBlock);
LXPBlock*           LXPBlock_ReadXPBlock    (LXPBlock* This);

void                LXPBlock_WriteN1        (LXPBlock* This, const void* inVal);
void                LXPBlock_WriteN2        (LXPBlock* This, const void* inVal);
void                LXPBlock_WriteN4        (LXPBlock* This, const void* inVal);
void                LXPBlock_WriteN8        (LXPBlock* This, const void* inVal);

void                LXPBlock_ReadN1         (LXPBlock* This, void* outVal);
void                LXPBlock_ReadN2         (LXPBlock* This, void* outVal);
void                LXPBlock_ReadN4         (LXPBlock* This, void* outVal);
void                LXPBlock_ReadN8         (LXPBlock* This, void* outVal);

void                LXPBlock_Seek           (LXPBlock* This, i4 inOffset, LXPBlock_TSeekMode inMode);
ui4                 LXPBlock_Tell           (LXPBlock* This);

void*               LXPBlock_GetData        (LXPBlock* This);
ui4                 LXPBlock_GetSize        (LXPBlock* This);

#define LXPBlock_Rewind(This) LXPBlock_Seek(This, 0, LXPBlock_START)

#endif


/* Copyright (C) 2001 Camil Demetrescu

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
