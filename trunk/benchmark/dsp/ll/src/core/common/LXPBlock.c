/* ============================================================================
 *  LXPBlock.c
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:58 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#include "LXPBlock.h"
#include "LException.h"
#include "LMemory.h"

struct LXPBlock {
    i1*             Block;
    ui4             CurrPos;
    ui4             Size;
    ui4             BlockSize;
};


#define BASE_SIZE   50

/* Exponential expansion: block size is expanded exponentially */
#define ExpandFunction_(arraySize)          ((arraySize)<<1);

/* In the current implementation the block size is equal to the
 * maximum size reached during the sequence of operations. The block size is never
 * reduced. */


/* ----------------------------------------------------------------------------
 *  New
 * ----------------------------------------------------------------------------
*/
LXPBlock* LXPBlock_New(){

    LXPBlock theObject;

    theObject.Block = (i1*)LMemory_Malloc(BASE_SIZE);

    theObject.Size      = BASE_SIZE;
    theObject.CurrPos   = 0;
    theObject.BlockSize = 0;

    return (LXPBlock*)LMemory_NewObject(LXPBlock,theObject);
}


/* ----------------------------------------------------------------------------
 *  NewFromData
 * ----------------------------------------------------------------------------
*/
LXPBlock* LXPBlock_NewFromData(void** inDataA, ui4 inSize){

    LXPBlock theObject;

    theObject.Block     = (i1*)*inDataA;
    theObject.Size      = inSize;
    theObject.BlockSize = inSize;
    theObject.CurrPos   = 0;

    /* Flag data segment is now incorporated into newly created LXPBlock */
    (*inDataA) = NULL;

    return LMemory_NewObject(LXPBlock,theObject);
}


/* ----------------------------------------------------------------------------
 *  Delete
 * ----------------------------------------------------------------------------
*/
void LXPBlock_Delete(LXPBlock** ThisA){

    LMemory_Free(&(*ThisA)->Block);

    LMemory_DeleteObject(ThisA);
}


/* ----------------------------------------------------------------------------
 *  Write
 * ----------------------------------------------------------------------------
*/
void LXPBlock_Write(LXPBlock* This, const void* inData, ui4 inSize){
    if (This->CurrPos+inSize > This->Size) {
        This->Size = ExpandFunction_(This->CurrPos+inSize);
        This->Block = (i1*)LMemory_Realloc(This->Block,This->Size);
    }
    LMemory_Copy(inData,This->Block+This->CurrPos,inSize);
    This->CurrPos+=inSize;
    if (This->CurrPos > This->BlockSize) This->BlockSize=This->CurrPos;
}


/* ----------------------------------------------------------------------------
 *  Read
 * ----------------------------------------------------------------------------
*/
void LXPBlock_Read(LXPBlock* This, void* outData, ui4 inSize){
    if (inSize>This->BlockSize-This->CurrPos) Throw(LXPBlock_UNEXPECTED_END_OF_BLOCK);
    LMemory_Copy(This->Block+This->CurrPos,outData,inSize);
    This->CurrPos+=inSize;
}


/* ----------------------------------------------------------------------------
 *  WriteXPBlock
 * ----------------------------------------------------------------------------
*/
void LXPBlock_WriteXPBlock(LXPBlock* This, const LXPBlock* inXPBlock){
    ui4 theSize = LXPBlock_GetSize((LXPBlock*)inXPBlock);
    LXPBlock_WriteN4(This,&theSize);
    LXPBlock_Write(This, LXPBlock_GetData((LXPBlock*)inXPBlock), theSize);    
}


/* ----------------------------------------------------------------------------
 *  ReadXPBlock
 * ----------------------------------------------------------------------------
*/
LXPBlock* LXPBlock_ReadXPBlock(LXPBlock* This){
    ui4   theSize;
    void* theData;
    LXPBlock_ReadN4(This, &theSize);
    theData = LMemory_Malloc(theSize);
    Try {
        LXPBlock_Read(This, theData, theSize);
    }
    CatchAny{
        LMemory_Free(&theData);
        Rethrow;
    }
    return LXPBlock_NewFromData(&theData, theSize);
}


/* ----------------------------------------------------------------------------
 *  Seek
 * ----------------------------------------------------------------------------
*/
void LXPBlock_Seek(LXPBlock* This, i4 inOffset, LXPBlock_TSeekMode inMode){
    switch (inMode) {
        case LXPBlock_START: 
            This->CurrPos = (inOffset<=0)? 0 : inOffset; 
            break;
        case LXPBlock_CURR:  
            This->CurrPos = (inOffset<=0 && ((ui4)-inOffset)>=This->CurrPos)? 0 : This->CurrPos+inOffset;
            break;
        case LXPBlock_END:   
            This->CurrPos = (inOffset<=0 && ((ui4)-inOffset)>=This->BlockSize)? 0 : This->BlockSize+inOffset;
            break;
    }
    if (This->CurrPos>This->BlockSize) This->CurrPos=This->BlockSize;
}


/* ----------------------------------------------------------------------------
 *  Tell
 * ----------------------------------------------------------------------------
*/
ui4 LXPBlock_Tell(LXPBlock* This){
    return This->CurrPos;
}

/* ----------------------------------------------------------------------------
 *  WriteN1
 * ----------------------------------------------------------------------------
*/
void LXPBlock_WriteN1(LXPBlock* This, const void* inVal){
    LXPBlock_Write(This, inVal, 1);
}


/* ----------------------------------------------------------------------------
 *  WriteN2
 * ----------------------------------------------------------------------------
*/
void LXPBlock_WriteN2(LXPBlock* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui2 theN2 = AtMem_(ui2,inVal);
    theN2 = SwapN2_(theN2);
    LXPBlock_Write(This, (void*)&theN2, 2);
#else
    LXPBlock_Write(This, inVal, 2);
#endif
}


/* ----------------------------------------------------------------------------
 *  WriteN4
 * ----------------------------------------------------------------------------
*/
void LXPBlock_WriteN4(LXPBlock* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4 = AtMem_(ui4,inVal);
    theN4 = SwapN4_(theN4);
    LXPBlock_Write(This, (void*)&theN4, 4);
#else
    LXPBlock_Write(This, inVal, 4);
#endif
}


/* ----------------------------------------------------------------------------
 *  WriteN8
 * ----------------------------------------------------------------------------
*/
void LXPBlock_WriteN8(LXPBlock* This, const void* inVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4a = AtMem_(ui4,inVal);
    ui4 theN4b = *(Mem_(ui4,inVal)+1);
    theN4a = SwapN4_(theN4a);
    theN4b = SwapN4_(theN4b);
    LXPBlock_Write(This, (void*)&theN4b, 4);
    LXPBlock_Write(This, (void*)&theN4a, 4);
#else
    LXPBlock_Write(This, inVal, 8);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN1
 * ----------------------------------------------------------------------------
*/
void LXPBlock_ReadN1(LXPBlock* This, void* outVal){
    LXPBlock_Read(This, outVal, 1);
}


/* ----------------------------------------------------------------------------
 *  ReadN2
 * ----------------------------------------------------------------------------
*/
void LXPBlock_ReadN2(LXPBlock* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui2 theN2;
    LXPBlock_Read(This, (void*)&theN2, 2);
    AtMem_(ui2,outVal) = SwapN2_(theN2);
#else
    LXPBlock_Read(This, outVal, 2);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN4
 * ----------------------------------------------------------------------------
*/
void LXPBlock_ReadN4(LXPBlock* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4;
    LXPBlock_Read(This, (void*)&theN4, 4);
    AtMem_(ui4,outVal) = SwapN4_(theN4);
#else
    LXPBlock_Read(This, outVal, 4);
#endif
}


/* ----------------------------------------------------------------------------
 *  ReadN8
 * ----------------------------------------------------------------------------
*/
void LXPBlock_ReadN8(LXPBlock* This, void* outVal){
#ifdef __LSL_LITTLE_ENDIAN__
    ui4 theN4a;
    ui4 theN4b;
    LXPBlock_Read(This, (void*)&theN4a, 4);
    LXPBlock_Read(This, (void*)&theN4b, 4);
    AtMem_(ui4,outVal)    = SwapN4_(theN4b);
    *(Mem_(ui4,outVal)+1) = SwapN4_(theN4a);
#else
    LXPBlock_Read(This, outVal, 8);
#endif
}


/* ----------------------------------------------------------------------------
 *  GetData
 * ----------------------------------------------------------------------------
*/
void* LXPBlock_GetData(LXPBlock* This){
    return This->Block;
}


/* ----------------------------------------------------------------------------
 *  GetDataSize
 * ----------------------------------------------------------------------------
*/
ui4 LXPBlock_GetSize(LXPBlock* This){
    return This->BlockSize;
}



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
