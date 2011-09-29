/* ============================================================================
 *  LDataStore.c
 * ============================================================================

 *  Author:         (c) 2001 Irene Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:48 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#include "LDataStore.h"
#include "LArray.h"
#include "LMemory.h"


/* MEMBER VARIABLES */
struct LDataStore {
    LArray* mData;
    LArray* mMap;
};


/* SHORTCUTS */
#define Map_(i)  AtMem_(ui4, LArray_ItemAt(This->mMap,i))


/* ---------------------------------------------------------------------------------
 *  New
 * ---------------------------------------------------------------------------------
*/
LDataStore* LDataStore_New(){

    LDataStore  theObject;
    ui4         theSentinel = 0;

    theObject.mData    = LArray_New(1);
    theObject.mMap     = LArray_New(4);
    LArray_AppendItem(theObject.mMap,&theSentinel);

    return LMemory_NewObject(LDataStore,theObject);
}


/* ---------------------------------------------------------------------------------
 *  NewFromData
 * ---------------------------------------------------------------------------------
*/
LDataStore* LDataStore_NewFromData(LXPBlock* thruBlock){

    void* theData;
    ui4*  theMap;
    ui4   i,theDataSize,theMapSize,theMapCount;
    ui4   theMagicNumber;
    ui4   theVersion;
    LDataStore theObject = {0};

    /* Check magic number */ 
    LXPBlock_ReadN4(thruBlock,(void*)&theMagicNumber);
    if (theMagicNumber!=LDataStore_MAGIC_NUMBER) 
        Throw(LDataStore_ILLEGAL_BLOCK_TYPE);

    /* Check file version */
    LXPBlock_ReadN4(thruBlock,(void*)&theVersion);
    if (theVersion > LDataStore_VERSION) 
        Throw(LDataStore_NEWER_BLOCK_VERSION);

    /* Read data and data map size */
    LXPBlock_ReadN4(thruBlock, &theDataSize);
    LXPBlock_ReadN4(thruBlock, &theMapSize);

    if (theDataSize>0) {
        /* Allocate memory for data */
        theData =       LMemory_Malloc(theDataSize);

        /* Read data from XPBlock */
        LXPBlock_Read(thruBlock, theData, theDataSize);

        /* Build new LArray object */
        theObject.mData = LArray_NewFromData(1,(void**)&theData,theDataSize);
    }
    else theObject.mData = LArray_New(1);
    
    /* Allocate memory for data map */
    theMap  = (ui4*)LMemory_Malloc(theMapSize);

    /* Read map from XPBlock */
    theMapCount = theMapSize>>2;
    for (i=0; i<theMapCount; i++) LXPBlock_ReadN4(thruBlock, theMap+i);

    /* Build new LArray object */
    theObject.mMap     = LArray_NewFromData(4,(void**)&theMap,theMapSize);

    return LMemory_NewObject(LDataStore,theObject);
}


/* ---------------------------------------------------------------------------------
 *  Delete
 * ---------------------------------------------------------------------------------
*/
void LDataStore_Delete(LDataStore** ThisA){

    LArray_Delete(&(*ThisA)->mData);
    LArray_Delete(&(*ThisA)->mMap);

    LMemory_DeleteObject(ThisA);
}


/* ---------------------------------------------------------------------------------
 *  AddBlock
 * ---------------------------------------------------------------------------------
*/
LDataStore_TIdx LDataStore_AddBlock(LDataStore* This, const void* inItem, ui4 inSize){

    LDataStore_TIdx theIdx;
    ui4             theBaseOffset;

    /* Access the sentinel */
    theIdx = LArray_GetItemsCount(This->mMap)-1;
    theBaseOffset = Map_(theIdx);

    /* Make room for the new block in the pool */
    LArray_ResizeBy(This->mData,(i4)inSize);

    /* Copy block to the pool */
    LMemory_Copy(inItem,LArray_ItemAt(This->mData,theBaseOffset),inSize);

    /* Update the sentinel */
    theBaseOffset+=inSize;
    LArray_AppendItem(This->mMap,&theBaseOffset);

    /* Return index of the newly created block in the pool */
    return theIdx;
}


/* ---------------------------------------------------------------------------------
 *  AddXPBlock
 * ---------------------------------------------------------------------------------
*/
LDataStore_TIdx LDataStore_AddXPBlock(LDataStore* This, const LXPBlock* inItem){
    return LDataStore_AddBlock(This, LXPBlock_GetData((LXPBlock*)inItem), LXPBlock_GetSize((LXPBlock*)inItem));
}


/* ---------------------------------------------------------------------------------
 *  AddUniqueBlock
 * ---------------------------------------------------------------------------------
*/
LDataStore_TIdx LDataStore_AddUniqueBlock(LDataStore* This, const void* inItem, ui4 inSize){

    LDataStore_TIdx theIdx = 0xFFFFFFFF;

    /* Check if the block already exists in the pool */
    if (LDataStore_GetBlockIndex(This,inItem,inSize,&theIdx)) return theIdx;

    /* If the block is not in the pool, add it and return index*/
    return LDataStore_AddBlock(This,inItem,inSize);
}


/* ---------------------------------------------------------------------------------
 *  GetBlockIndex
 * ---------------------------------------------------------------------------------
*/
Bool LDataStore_GetBlockIndex(LDataStore* This, const void* inItem, ui4 inSize, LDataStore_TIdx* outIndex){

    LDataStore_TIdx theIdx;
    ui4             theBlocksCount;

    theBlocksCount = LArray_GetItemsCount(This->mMap)-1;

    /* Scan blocks */
    for (theIdx=0; theIdx<theBlocksCount; theIdx++){
        if (inSize!=Map_(theIdx+1)-Map_(theIdx)) continue;
        if (!LMemory_Compare(inItem,
                             LArray_ItemAt(This->mData,
                             LDataStore_GetOffsetByIndex(This,theIdx)),inSize)) 
            return (*outIndex=theIdx,TRUE);
    }

    /* The block has not been found */
    return FALSE;
}


/* ---------------------------------------------------------------------------------
 *  GetOffsetByIndex
 * ---------------------------------------------------------------------------------
*/
ui4 LDataStore_GetOffsetByIndex(LDataStore* This, LDataStore_TIdx inIndex){
    return Map_(inIndex);
}


/* ---------------------------------------------------------------------------------
 *  GetSizeByIndex
 * ---------------------------------------------------------------------------------
*/
ui4 LDataStore_GetSizeByIndex(LDataStore* This, LDataStore_TIdx inIndex){
    return Map_(inIndex+1)-Map_(inIndex);
}


/* ---------------------------------------------------------------------------------
 *  FetchBlockByIndex
 * ---------------------------------------------------------------------------------
*/
void LDataStore_FetchBlockByIndex(LDataStore* This, LDataStore_TIdx inIndex, void* outItem){
    ui4 theOffset = Map_(inIndex);
    ui4 theSize   = Map_(inIndex+1)-Map_(inIndex);
    i1* theData   = (i1*)LArray_GetData(This->mData);
    LMemory_Copy(theData+theOffset,outItem,theSize);
}


/* ---------------------------------------------------------------------------------
 *  GetDataByIndex
 * ---------------------------------------------------------------------------------
*/
void* LDataStore_GetDataByIndex(LDataStore* This, LDataStore_TIdx inIndex){
    return (void*) ((i1*)LArray_GetData(This->mData) + Map_(inIndex));
}


/* ---------------------------------------------------------------------------------
 *  GetXPBlockByIndex
 * ---------------------------------------------------------------------------------
*/
LXPBlock* LDataStore_GetXPBlockByIndex(LDataStore* This, LDataStore_TIdx inIndex){
    void*     theData;
    ui4       theSize;
    theSize = Map_(inIndex+1)-Map_(inIndex);
    theData = LMemory_Malloc(theSize);
    LDataStore_FetchBlockByIndex(This, inIndex, theData);
    return LXPBlock_NewFromData(&theData, theSize);
}


/* ---------------------------------------------------------------------------------
 *  GetData
 * ---------------------------------------------------------------------------------
*/
void* LDataStore_GetData(LDataStore* This){
    return LArray_GetData(This->mData);
}


/* ---------------------------------------------------------------------------------
 *  GetSize
 * ---------------------------------------------------------------------------------
*/
ui4 LDataStore_GetSize(LDataStore* This){
    return LArray_GetDataSize(This->mData);
}


/* ---------------------------------------------------------------------------------
 *  GetMap
 * ---------------------------------------------------------------------------------
*/
void* LDataStore_GetMap(LDataStore* This){
    return LArray_GetData(This->mMap);
}


/* ---------------------------------------------------------------------------------
 *  GetMapSize
 * ---------------------------------------------------------------------------------
*/
ui4 LDataStore_GetMapSize(LDataStore* This){
    return LArray_GetDataSize(This->mMap);
}


/* ---------------------------------------------------------------------------------
 *  GetBlocksCount
 * ---------------------------------------------------------------------------------
*/
ui4 LDataStore_GetBlocksCount(LDataStore* This){
    return LArray_GetItemsCount(This->mMap)-1;
}


/* ---------------------------------------------------------------------------------
 *  GetXPBlock
 * ---------------------------------------------------------------------------------
*/
LXPBlock* LDataStore_GetXPBlock(LDataStore* This){

    LXPBlock*   theBlock;
    ui4         i, theVersion, theMagicNumber;

    i1*         theData     = (i1*) LDataStore_GetData(This);
    ui4         theDataSize =       LDataStore_GetSize(This);
    ui4*        theMap      = (ui4*)LDataStore_GetMap(This);
    ui4         theMapSize  =       LDataStore_GetMapSize(This);
    ui4         theMapCount =       theMapSize >> 2;

    /* Create a new LXPBlock() */
    theBlock = LXPBlock_New();

    /* Write MAGIC NUMBER */
    theMagicNumber = LDataStore_MAGIC_NUMBER;
    LXPBlock_WriteN4(theBlock,&theMagicNumber);

    /* Write VERSION */
    theVersion = LDataStore_VERSION;
    LXPBlock_WriteN4(theBlock,&theVersion);

    /* Write data and map size to XPBlock */
    LXPBlock_WriteN4(theBlock, &theDataSize);
    LXPBlock_WriteN4(theBlock, &theMapSize);

    /* Write data to XPBlock: assume data is portable */
    LXPBlock_Write(theBlock, theData, theDataSize);

    /* Write map to XPBlock */
    for (i=0; i<theMapCount; i++) LXPBlock_WriteN4(theBlock, theMap+i);

    /* Set block current position to beginning */
    LXPBlock_Rewind(theBlock);

    return theBlock;
}


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

