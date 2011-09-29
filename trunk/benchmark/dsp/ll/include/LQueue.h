/* ============================================================================
 *  LQueue.h
 * ============================================================================

 *  Author:         (C) 2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 20, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:23 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LQueue__
#define __LQueue__

/* INCLUDES */
#include "LType.h"

/* COMPONENT ID */
#define LQueue_ID    0x802E

/* EXCEPTION CODES */
enum {
    LQueue_EMPTY_QUEUE = LQueue_ID<<16,
    LQueue_ITEM_TYPE_MISMATCH
};

/* TYPEDEFS */
typedef struct LQueue LQueue;

/* PUBLIC FUNCTIONS */
LQueue*      LQueue_New           (LType_TType inType);
void         LQueue_Delete        (LQueue** ThisA);

Bool         LQueue_IsEmpty       (LQueue*);

void         LQueue_EnqueueI1     (LQueue* This,    i1 inVal);       
void         LQueue_EnqueueUI1    (LQueue* This,   ui1 inVal);       
void         LQueue_EnqueueI2     (LQueue* This,    i2 inVal);       
void         LQueue_EnqueueUI2    (LQueue* This,   ui2 inVal);       
void         LQueue_EnqueueI4     (LQueue* This,    i4 inVal);       
void         LQueue_EnqueueUI4    (LQueue* This,   ui4 inVal);       
void         LQueue_EnqueueF4     (LQueue* This,    f4 inVal);       
void         LQueue_EnqueueF8     (LQueue* This,    f8 inVal);       
void         LQueue_EnqueueBool   (LQueue* This,  Bool inVal);       
void         LQueue_EnqueuePtr    (LQueue* This, void* inVal);       
void         LQueue_EnqueueItem   (LQueue* This, const void* inItem);

i1           LQueue_DequeueI1     (LQueue* This);       
ui1          LQueue_DequeueUI1    (LQueue* This);       
i2           LQueue_DequeueI2     (LQueue* This);       
ui2          LQueue_DequeueUI2    (LQueue* This);       
i4           LQueue_DequeueI4     (LQueue* This);       
ui4          LQueue_DequeueUI4    (LQueue* This);       
f4           LQueue_DequeueF4     (LQueue* This);       
f8           LQueue_DequeueF8     (LQueue* This);       
Bool         LQueue_DequeueBool   (LQueue* This);       
void*        LQueue_DequeuePtr    (LQueue* This);       
void         LQueue_DequeueItem   (LQueue* This, void* outItem);

ui4          LQueue_GetUsedMem    (LQueue* This);
LType_TType  LQueue_GetItemType   (LQueue* This);

#endif

/* Copyright (C) 2003 Camil Demetrescu

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

