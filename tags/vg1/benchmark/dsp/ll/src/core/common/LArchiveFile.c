/* ============================================================================
 *  LArchivefile.c
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu, Francesco Mungiguerra
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LArchiveFile.h"
#include "LFile.h"
#include "LMemory.h"
#include "LException.h"
#include "LXPBlock.h"
#include "LDebug.h"
#include "LType.h"
#include "LString.h"

/* set to 1 to enable debugging output */
#define TRACE_EXECUTION_             0

#define NULL_OFFSET                  0
#define FIRST_TAG_OFFSET_OFFSET      8
#define FIRST_VALID_OFFSET          12
#define BLOCK_HEADER_SIZE           12
#define TAG_OFFSET_OFFSET            4
#define DEFRAG_THRESHOLD           0.2

#define BEST_FIT_                    0
#define FIRST_FIT_                   1
#define WORST_FIT_                   2

#define STORAGE_ALLOCATION_STRATEGY_    FIRST_FIT_   

/* MEMBER VARIABLES */
struct LArchiveFile {
    LFile*                  File;
    LArray*                 BlocksMap;
    LArchiveFile_TOpenMode  Mode;
    Bool                    Defrag;
    i1                      FileName[LFile_MAX_PATHNAME_LEN];
};

/* PRIVATE TYPES */
typedef struct {
    ui4 Tag;
    ui4 Offset;
    ui4 BlockSize;
} TBlockHeader;

/* PRIVATE FUNCTIONS */
f8      _GetFragmentation   (LArchiveFile* This);
void    _Compact            (const i1* theFileName);
void    _FetchHoleOffset(LArchiveFile* This,
                         ui4  inRequestSize,
                         ui4* outBlockIdx, 
                         ui4* outNextOffsetOffset, 
                         ui4* outNextOffset, 
                         ui4* outHoleOffset);


/* ---------------------------------------------------------------------------------
 *  Open
 * ---------------------------------------------------------------------------------
 * Constructor */

LArchiveFile* LArchiveFile_Open(const i1* inFileName, LArchiveFile_TOpenMode inMode) {

    LArchiveFile    Object = { 0 };

    LException*     theException;
    Bool            theFileExists;
    ui4             theOffset;
    ui4             theMagicNumber;
    ui4             theVersion;
    ui4             theFirstTagOffset;

    Try {

        theFileExists = LFile_Exists(inFileName);
    
        /* Store local copy od file name */
        LString_Copy(inFileName, Object.FileName);
    
        /* Store access mode */
        Object.Mode     = inMode;
    
        /* Set flag to force defragmentation upon close operation */
        Object.Defrag   = TRUE;
    
        /* File exists and read/update mode */
        if (theFileExists && (inMode==LArchiveFile_READ || inMode==LArchiveFile_READ_WRITE)){
    
            Object.File = LFile_Open(inFileName,(LFile_TOpenMode)inMode);
    
            /* Check magic number */ 
            LFile_ReadN4(Object.File, (void*)&theMagicNumber);
            if(theMagicNumber!=LArchiveFile_MAGIC_NUMBER) 
                Throw(LArchiveFile_ILLEGAL_FILE_TYPE);
    
            /* Check file version */
            LFile_ReadN4(Object.File, (void*)&theVersion);
            if(theVersion > LArchiveFile_VERSION) 
                Throw(LArchiveFile_NEWER_FILE_VERSION);
    
            /* Check file integrity and read first tag offset */
            LFile_ReadN4(Object.File,(void*)&theOffset);
            
            /* Create block map */
            Object.BlocksMap = LArray_New(sizeof(TBlockHeader));
            
            /* Scan disk and build block map */
            while(theOffset){
    
                TBlockHeader theBlockInfo;
    
                if (!LFile_Seek(Object.File,theOffset,LFile_START)) 
                    Throw(LArchiveFile_DAMAGED_FILE);
    
                theBlockInfo.Offset = theOffset;    
                LFile_ReadN4(Object.File,(void*)&(theBlockInfo.Tag));
                LFile_ReadN4(Object.File,(void*)&(theOffset));
                LFile_ReadN4(Object.File,(void*)&(theBlockInfo.BlockSize));
                
                LArray_AppendItem(Object.BlocksMap,(void*)&theBlockInfo);           
            }
        } /* *** end *** */
    
        /* Update mode with non-existing file or write mode
        * Create the file and initialize the default header */
        else if((inMode==LArchiveFile_WRITE) || (inMode==LArchiveFile_READ_WRITE && !theFileExists)){
        
            Object.File = LFile_Open(inFileName,(LFile_TOpenMode)inMode);
    
            /* Write Magic Number */
            theMagicNumber = LArchiveFile_MAGIC_NUMBER;
            LFile_WriteN4(Object.File,(void*)&theMagicNumber);
            
            /* Write file version */
            theVersion = LArchiveFile_VERSION;
            LFile_WriteN4(Object.File,(void*)&theVersion);
            
            /* write the default first tag offset */
            theFirstTagOffset=NULL_OFFSET;
            LFile_WriteN4(Object.File,(void*)&theFirstTagOffset);
            
            /* create LArray structure for the file */
            Object.BlocksMap = LArray_New(sizeof(TBlockHeader));
            
        } /* *** end *** */
    
        /* File does not exist and read mode */
        else if(inMode==LArchiveFile_READ && !theFileExists){
        
            Throw(LArchiveFile_CANT_OPEN_FILE);
        
        } /* *** end *** */
    }
    Catch(theException) {
        /* memory cleanup */
        if (Object.BlocksMap!=NULL) LArray_Delete(&Object.BlocksMap);
        if (Object.File!=NULL)      LFile_Close(&Object.File);
        Rethrow;
    }

    return LMemory_NewObject(LArchiveFile,Object);
}


/* ---------------------------------------------------------------------------------
 *  Close
 * ---------------------------------------------------------------------------------
 * Destructor */

void LArchiveFile_Close(LArchiveFile** ThisA) {

    f8 theFrag;
    
    /* compute the fragmentation of the file */
    if ((*ThisA)->Defrag) theFrag = _GetFragmentation((*ThisA));

    /* close file */
    LFile_Close(&(*ThisA)->File); 

    /* compact file in case of high fragmentation */
    if ((*ThisA)->Defrag && theFrag>DEFRAG_THRESHOLD) {
        #if TRACE_EXECUTION_
        LDebug_Print("\nCompacting...");  
        #endif
        _Compact((*ThisA)->FileName);
    }

    /* deallocate map array */
    LArray_Delete(&(*ThisA)->BlocksMap);

    /* deallocate object */
    LMemory_DeleteObject(ThisA);
}


/* ---------------------------------------------------------------------------------
 *  AddBlock
 * ---------------------------------------------------------------------------------
*/

ui4 LArchiveFile_AddBlock(LArchiveFile* This, ui4 inBlockTag, LXPBlock* inTagBlock) {

    TBlockHeader    theBlockInfo;
    ui4             theNextOffsetOffset;
    ui4             theNextOffset;
    ui4             theHoleOffset;
    ui4             theBlockSize;
    ui4             theBlockIdx;

    /* Access mode check */
    if (This->Mode==LArchiveFile_READ) Throw(LArchiveFile_READ_ONLY_ACCESS);

    theBlockSize = LXPBlock_GetSize(inTagBlock);

    /* Get hole offset (if any) */
    _FetchHoleOffset(This, BLOCK_HEADER_SIZE+theBlockSize, 
                     &theBlockIdx, &theNextOffsetOffset, &theNextOffset, &theHoleOffset);
                     
    /* Make previous block point to the new block */
    LFile_Seek(This->File,theNextOffsetOffset,LFile_START);
    LFile_WriteN4(This->File,(void*)&theHoleOffset);
    
    /* Write new block */

    /* seek the offset of the new block */
    LFile_Seek(This->File,theHoleOffset,LFile_START);
    
    /* write the block header and the block */
    LFile_WriteN4(This->File,(void*)&inBlockTag);
    LFile_WriteN4(This->File,(void*)&theNextOffset);
    LFile_WriteN4(This->File,(void*)&theBlockSize);
    LFile_WriteXPBlock(This->File,inTagBlock);
    
    /* Update the array structure */
    theBlockInfo.Tag        = inBlockTag;
    theBlockInfo.Offset     = theHoleOffset;
    theBlockInfo.BlockSize  = theBlockSize;
    
    /* insert the item in the LArray */         
    LArray_InsertItemAt(This->BlocksMap, (void*)&theBlockInfo, theBlockIdx);
    
    return theBlockIdx;
}


/* ---------------------------------------------------------------------------------
 *  RemoveBlock
 * ---------------------------------------------------------------------------------
*/

void LArchiveFile_RemoveBlock(LArchiveFile* This, ui4 inBlockIdx) {

    ui4 theNewOffset;
    ui4 thePrevOffsetOffset;

    /* Range check */
    if (inBlockIdx>=LArray_GetItemsCount(This->BlocksMap)) Throw(LArchiveFile_OUT_OF_RANGE);

    /* Access mode check */
    if (This->Mode==LArchiveFile_READ) Throw(LArchiveFile_READ_ONLY_ACCESS);

    /* If the block is the last */
    if (inBlockIdx==LArray_GetItemsCount(This->BlocksMap)-1) 
         theNewOffset = NULL_OFFSET; 
    else theNewOffset = ((TBlockHeader*)LArray_ItemAt(This->BlocksMap,inBlockIdx+1))->Offset;

    /* If the block is the first */
    if (inBlockIdx==0)
         thePrevOffsetOffset = FIRST_TAG_OFFSET_OFFSET;
    else thePrevOffsetOffset = ((TBlockHeader*)LArray_ItemAt(This->BlocksMap,inBlockIdx-1))->Offset+4;

    /* Move current position to the offset field to be updated */
    if(!LFile_Seek(This->File,thePrevOffsetOffset,LFile_START))
        Throw(LArchiveFile_DAMAGED_FILE);

    /* Overwrite the offset of the canceled block */
    LFile_WriteN4(This->File,(void*)&theNewOffset);
    
    /* Remove item with index inBlockIdx */
    LArray_RemoveItemAt(This->BlocksMap,inBlockIdx);
}


/* ---------------------------------------------------------------------------------
 *  GetBlockTag
 * ---------------------------------------------------------------------------------
*/

ui4 LArchiveFile_GetBlockTag(LArchiveFile* This, ui4 inBlockIdx) {

    TBlockHeader* theBlockInfoPtr;

    /* Range check */
    if (inBlockIdx >= LArray_GetItemsCount(This->BlocksMap)) 
        Throw(LArchiveFile_OUT_OF_RANGE);
 
    /* Access mode check */
    if (This->Mode == LArchiveFile_WRITE) 
        Throw(LArchiveFile_WRITE_ONLY_ACCESS);

    /* Extract the item */
    theBlockInfoPtr = (TBlockHeader*)LArray_ItemAt(This->BlocksMap,inBlockIdx);
    
    /* Return the block */
    return theBlockInfoPtr->Tag;
}


/* ---------------------------------------------------------------------------------
 *  GetXPBlock
 * ---------------------------------------------------------------------------------
*/

LXPBlock* LArchiveFile_GetXPBlock(LArchiveFile* This, ui4 inBlockIdx) {

    TBlockHeader*   theBlockInfoPtr;
    LXPBlock*       theBlock;
    
    /* Range check */
   if (inBlockIdx >= LArray_GetItemsCount(This->BlocksMap)) 
        Throw(LArchiveFile_OUT_OF_RANGE);

    /* Access mode check */
    if (This->Mode==LArchiveFile_WRITE) 
        Throw(LArchiveFile_WRITE_ONLY_ACCESS);

    /* Extract the item */
    theBlockInfoPtr = (TBlockHeader*)LArray_ItemAt(This->BlocksMap,inBlockIdx);
    
    /* Seek the beginning of the block */
    if((!LFile_Seek(This->File,theBlockInfoPtr->Offset+3*sizeof(ui4),LFile_START))) 
        Throw(LArchiveFile_DAMAGED_FILE);
    
    /* Return the block */
    theBlock = LFile_ReadXPBlock(This->File, theBlockInfoPtr->BlockSize);
    
    return theBlock;
}


/* ---------------------------------------------------------------------------------
 *  GetBlocksCount
 * ---------------------------------------------------------------------------------
*/

ui4 LArchiveFile_GetBlocksCount(LArchiveFile* This) {
    return LArray_GetItemsCount(This->BlocksMap);
}


/* ---------------------------------------------------------------------------------
 *  _FetchHoleOffset
 * ---------------------------------------------------------------------------------
*/

void _FetchHoleOffset(LArchiveFile* This,
                      ui4  inRequestSize,
                      ui4* outBlockIdx, 
                      ui4* outNextOffsetOffset, 
                      ui4* outNextOffset, 
                      ui4* outHoleOffset) {

    ui4 theItemsCount;
    ui4 theHoleSize;

    #if   STORAGE_ALLOCATION_STRATEGY_  ==  BEST_FIT_
    ui4 theBestNextOffsetOffset;
    ui4 theBestNextOffset;
    ui4 theBestHoleOffset;
    ui4 theBestBlockIdx;
    ui4 theBestHoleSize;
    #endif

    theItemsCount = LArray_GetItemsCount(This->BlocksMap);

    *outNextOffsetOffset = FIRST_TAG_OFFSET_OFFSET;
    *outNextOffset       = (theItemsCount==0) ? 0 : ((TBlockHeader*)LArray_ItemAt(This->BlocksMap,0))->Offset;
    *outHoleOffset       = FIRST_VALID_OFFSET;
    *outBlockIdx         = 0;

    #if STORAGE_ALLOCATION_STRATEGY_  ==  BEST_FIT_
    theBestNextOffsetOffset = *outNextOffsetOffset;
    theBestNextOffset       = *outNextOffset;
    theBestHoleOffset       = *outHoleOffset;
    theBestBlockIdx         = *outBlockIdx;
    theBestHoleSize         = 0xFFFFFFFF;
    #endif

    while ( *outBlockIdx<theItemsCount ) {

        theHoleSize = *outNextOffset - *outHoleOffset;

        #if   STORAGE_ALLOCATION_STRATEGY_  ==  FIRST_FIT_   

        if (theHoleSize>=inRequestSize) break;

        #elif STORAGE_ALLOCATION_STRATEGY_  ==  BEST_FIT_

        if (theHoleSize>=inRequestSize && theHoleSize<theBestHoleSize) {
            theBestNextOffsetOffset = *outNextOffsetOffset;
            theBestNextOffset       = *outNextOffset;
            theBestHoleOffset       = *outHoleOffset;
            theBestBlockIdx         = *outBlockIdx;
            theBestHoleSize         = theHoleSize;
        }

        #endif

        *outHoleOffset       = *outNextOffset+ BLOCK_HEADER_SIZE+
                                    ((TBlockHeader*)LArray_ItemAt(This->BlocksMap, *outBlockIdx))->BlockSize;
        *outBlockIdx         += 1;
        *outNextOffsetOffset = *outNextOffset+TAG_OFFSET_OFFSET;
        *outNextOffset       = (*outBlockIdx==theItemsCount) ? 
                                    NULL_OFFSET : 
                                    ((TBlockHeader*)LArray_ItemAt(This->BlocksMap, *outBlockIdx))->Offset;
    }

    #if STORAGE_ALLOCATION_STRATEGY_  ==  BEST_FIT_
    *outNextOffsetOffset = theBestNextOffsetOffset;
    *outNextOffset       = theBestNextOffset;
    *outHoleOffset       = theBestHoleOffset;
    *outBlockIdx         = theBestBlockIdx;
    #endif

    return;
}


/* ---------------------------------------------------------------------------------
 *  _Compact
 * ---------------------------------------------------------------------------------
*/

void _Compact(const i1* theFileName) {

    ui4            theBlocksCount;
    ui4            theBlockIdx;
    i1             theTempFileName[LFile_MAX_PATHNAME_LEN];
    LArchiveFile*  theArchiveFile;
    LArchiveFile*  theTempArchiveFile;

    /* get temporary file name */
    LFile_GetTempName(theTempFileName);

    /* open file to be defragmented */
    theArchiveFile     = LArchiveFile_Open(theFileName, LArchiveFile_READ );
    theBlocksCount     = LArchiveFile_GetBlocksCount(theArchiveFile);

    /* create temporary archive file */
    theTempArchiveFile = LArchiveFile_Open(theTempFileName, LArchiveFile_WRITE);

    /* scan and copy blocks to temp file */
    for (theBlockIdx=0 ; theBlockIdx<theBlocksCount ; theBlockIdx++ ) {
    
        LXPBlock* theBlock = LArchiveFile_GetXPBlock(theArchiveFile,theBlockIdx);
    
        LArchiveFile_AddBlock(theTempArchiveFile,
                                LArchiveFile_GetBlockTag(theArchiveFile,theBlockIdx),
                                theBlock);

        /* deallocate */
        LXPBlock_Delete(&theBlock); 
    }

    /* turn off defragmentation to avoid recursive calls of _Compact() */
    theArchiveFile->Defrag      = FALSE;
    theTempArchiveFile->Defrag  = FALSE;

    /* close source and temporary files */
    LArchiveFile_Close(&theArchiveFile);
    LArchiveFile_Close(&theTempArchiveFile);

    #if TRACE_EXECUTION_
    LDebug_Print("\nRemoving file : %s",theFileName);
    LDebug_Print("\nRenaming file %s to %s",theTempFileName,theFileName);
    #endif

    /* remove the old file and rename the compacted temp file */
    LFile_Remove(theFileName);
    LFile_Rename(theTempFileName, theFileName);
}


/* ---------------------------------------------------------------------------------
 *  _GetFragmentation
 * ---------------------------------------------------------------------------------
 * Compute fragmentation as sum of unused space/file size
*/

f8 _GetFragmentation(LArchiveFile* This) {

    ui4 theItemsCount;
    ui4 theBlankSpace;
    ui4 outNextOffset;
    ui4 outHoleOffset;
    ui4 outBlockIdx;

    theItemsCount = LArray_GetItemsCount(This->BlocksMap);
    
    outNextOffset        = (theItemsCount==0) ? 0 : ((TBlockHeader*)LArray_ItemAt(This->BlocksMap,0))->Offset;
    outHoleOffset        = FIRST_VALID_OFFSET;
    outBlockIdx          = 0;
    theBlankSpace        = 0;
    
    while ( outBlockIdx<theItemsCount ) {

        theBlankSpace = theBlankSpace + (outNextOffset - outHoleOffset);
        
        #if TRACE_EXECUTION_
        LDebug_Print("\ntheBlankSpace : %u",theBlankSpace);
        #endif

        outHoleOffset        = outNextOffset+ BLOCK_HEADER_SIZE+
                                    ((TBlockHeader*)LArray_ItemAt(This->BlocksMap, outBlockIdx))->BlockSize;
        outBlockIdx         += 1;
        outNextOffset        = (outBlockIdx==theItemsCount) ? 
                                    NULL_OFFSET : 
                                    ((TBlockHeader*)LArray_ItemAt(This->BlocksMap, outBlockIdx))->Offset;
    }
    
    #if TRACE_EXECUTION_
    LDebug_Print("\ntheBlankSpace Tot: %u",theBlankSpace);
    LDebug_Print("\ntheFileSize : %u",LFile_GetSize(This->File));
    LDebug_Print("\nFragmentation : %lf\n",((f8)theBlankSpace)/(f8)(LFile_GetSize(This->File)));
    #endif
    
    return ( (f8)theBlankSpace/(f8)LFile_GetSize(This->File) );
}


/* Copyright (C) 2001 Camil Demetrescu, Francesco Mungiguerra

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
