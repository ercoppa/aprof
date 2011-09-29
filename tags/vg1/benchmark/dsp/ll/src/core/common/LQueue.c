/* ============================================================================
 *  LQueue.c
 * ============================================================================

 *  Author:         (C) 2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        January 20, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:55 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LQueue.h"
#include "LArray.h"
#include "LMemory.h"
#include "LException.h"

/* TYPES */
struct LQueue {
    LArray*      mArray;
    i1*          mData;
    ui4          mFront;
    ui4          mRear;
    LType_TType  mItemType;
};


/* MACROS */
#define INIT_QUEUE_SIZE 256
#define ItemAt_(i) ((void*)(This->mData + (i) * This->mItemType.mSize))

/* PRIVATE FUNCTIONS */
static void* _Enqueue(LQueue* This);
static void* _Dequeue(LQueue* This);


/* PUBLIC FUNCTIONS */

/* ---------------------------------------------------------------------------------
 *  New
 * ---------------------------------------------------------------------------------
*/
LQueue* LQueue_New(LType_TType inType){
    LQueue theObject = { 0 };

    Try {
        theObject.mItemType = inType;
        theObject.mFront    = 0;
        theObject.mRear     = 0;
        theObject.mArray    = LArray_New(inType.mSize);
        LArray_InstallSyncPtr(theObject.mArray, (void**)&theObject.mData);
        LArray_ResizeBy(theObject.mArray, INIT_QUEUE_SIZE);
    }
    CatchAny {
        if (theObject.mArray != NULL) LArray_Delete(&theObject.mArray);
        Rethrow;
    }

    return LMemory_NewObject(LQueue, theObject);
}

/* ---------------------------------------------------------------------------------
 *  Delete
 * ---------------------------------------------------------------------------------
*/
void LQueue_Delete(LQueue** ThisA){
    LArray_Delete(&(*ThisA)->mArray);
    LMemory_DeleteObject(ThisA);
}

/* ---------------------------------------------------------------------------------
 *  IsEmpty
 * ---------------------------------------------------------------------------------
*/
Bool LQueue_IsEmpty(LQueue* This){
    return (Bool)(This->mFront == This->mRear);
}

/* ---------------------------------------------------------------------------------
 *  EnqueueI1
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueI1(LQueue* This, i1 inVal){ 
    if (!LType_EqualTypes(This->mItemType, LType_I1)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _i1_(_Enqueue(This)) = inVal; 
}

/* ---------------------------------------------------------------------------------
 *  EnqueueUI1
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueUI1(LQueue* This, ui1 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_UI1)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _ui1_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueI2
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueI2(LQueue* This, i2 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_I2)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _i2_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueUI2
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueUI2(LQueue* This, ui2 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_UI2)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _ui2_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueI4
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueI4(LQueue* This, i4 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_I4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _i4_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueUI4
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueUI4(LQueue* This, ui4 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_UI4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _ui4_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueF4
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueF4(LQueue* This, f4 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_F4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _f4_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueF8
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueF8(LQueue* This, f8 inVal){
    if (!LType_EqualTypes(This->mItemType, LType_F8)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    _f8_(_Enqueue(This)) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueBool
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueBool(LQueue* This, Bool inVal){
    if (!LType_EqualTypes(This->mItemType, LType_Bool)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    *(Bool*)_Enqueue(This) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueuePointer
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueuePtr(LQueue* This, void* inVal){
    if (!LType_EqualTypes(This->mItemType, LType_Ptr)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    *(void**)_Enqueue(This) = inVal;
}

/* ---------------------------------------------------------------------------------
 *  EnqueueItem
 * ---------------------------------------------------------------------------------
*/
void LQueue_EnqueueItem(LQueue* This, const void* inItem){
    LMemory_Copy(inItem, _Enqueue(This), This->mItemType.mSize);
}

/* ---------------------------------------------------------------------------------
 *  DequeueI1
 * ---------------------------------------------------------------------------------
*/
i1 LQueue_DequeueI1(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_I1)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _i1_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueUI1
 * ---------------------------------------------------------------------------------
*/
ui1 LQueue_DequeueUI1(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_UI1)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _ui1_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueI2
 * ---------------------------------------------------------------------------------
*/
i2 LQueue_DequeueI2(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_I2)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _i2_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueUI2
 * ---------------------------------------------------------------------------------
*/
ui2 LQueue_DequeueUI2(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_UI2)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _ui2_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueI4
 * ---------------------------------------------------------------------------------
*/
i4 LQueue_DequeueI4(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_I4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _i4_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueUI4
 * ---------------------------------------------------------------------------------
*/
ui4 LQueue_DequeueUI4(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_UI4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _ui4_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueF4
 * ---------------------------------------------------------------------------------
*/
f4 LQueue_DequeueF4(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_F4)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _f4_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueF8
 * ---------------------------------------------------------------------------------
*/
f8 LQueue_DequeueF8(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_F8)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return _f8_(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueBool
 * ---------------------------------------------------------------------------------
*/
Bool LQueue_DequeueBool(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_Bool)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return *(Bool*)(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeuePointer
 * ---------------------------------------------------------------------------------
*/
void* LQueue_DequeuePtr(LQueue* This){
    if (!LType_EqualTypes(This->mItemType, LType_Ptr)) Throw(LQueue_ITEM_TYPE_MISMATCH);
    return *(void**)(_Dequeue(This));
}

/* ---------------------------------------------------------------------------------
 *  DequeueItem
 * ---------------------------------------------------------------------------------
*/
void LQueue_DequeueItem(LQueue* This, void* outItem){
     LMemory_Copy(_Dequeue(This), outItem, This->mItemType.mSize);
}

/* ---------------------------------------------------------------------------------
 *  GetUsedMem
 * ---------------------------------------------------------------------------------
*/
ui4 LQueue_GetUsedMem(LQueue* This){
    return LArray_GetUsedMem(This->mArray) + sizeof(LQueue);
}

/* ---------------------------------------------------------------------------------
 *  GetItemType
 * ---------------------------------------------------------------------------------
*/
LType_TType LQueue_GetItemType(LQueue* This){
    return This->mItemType;
}


/* PRIVATE FUNCTIONS */

/* ---------------------------------------------------------------------------------
 *  _Enqueue
 * ---------------------------------------------------------------------------------
*/
void* _Enqueue(LQueue* This) {
    void* theItem;
    ui4 theItemsCount = LArray_GetItemsCount(This->mArray);
    if ((This->mFront+1) % theItemsCount == This->mRear) {
        LArray_ResizeBy(This->mArray, (i4)theItemsCount);
        if (This->mFront < This->mRear) {
            if (This->mFront>0) 
                LMemory_Copy(ItemAt_(0), 
                             ItemAt_(theItemsCount), 
                             This->mFront * This->mItemType.mSize);
            This->mFront += theItemsCount;
        }
    }
    theItem = ItemAt_(This->mFront);
    This->mFront = (This->mFront+1) % LArray_GetItemsCount(This->mArray);
    return theItem;
}

/* ---------------------------------------------------------------------------------
 *  _Dequeue
 * ---------------------------------------------------------------------------------
*/
static void* _Dequeue(LQueue* This) {
    void* theItem;
    if (This->mFront == This->mRear) Throw(LQueue_EMPTY_QUEUE);
    theItem = ItemAt_(This->mRear);
    This->mRear = (This->mRear+1) % LArray_GetItemsCount(This->mArray);
    return theItem;
}


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
