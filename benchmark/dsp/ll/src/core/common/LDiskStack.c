/* ============================================================================
 *  LDiskStack.c
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu, Irene Finocchi, Francesco Mungiguerra
 *  License:        See the end of this file for license information
 *  Created:        March 27, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:49 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#include "LFile.h"
#include "LMemory.h"
#include "LException.h"
#include "LType.h"
#include "LDiskStack.h"


/* MEMBER VARIABLES */
struct LDiskStack {
    ui4         MemBufferSize;          
    void*       MemBufferBase;          
    void*       MemTop;
    i1          FileBufferName[LFile_MAX_PATHNAME_LEN];
    LFile*      FileBuffer;
    ui4         FileBufferSize;
};

/* ---------------------------------------------------------------------------------
 *  New
 * ---------------------------------------------------------------------------------
 * Constructor */

LDiskStack* LDiskStack_New(const ui4 inBufferSize) {

    /* Create the object LDiskStack */
    LDiskStack Object;
    
   /* Set the dimension of the stack */
    Object.MemBufferSize = inBufferSize;

   /* Allocate the memory buffer */
    Object.MemBufferBase = LMemory_Malloc(Object.MemBufferSize);

    /* Exception handling */
    if( Object.MemBufferBase==NULL)
        Throw(LMemory_OUT_OF_MEMORY);

    Object.MemTop = Object.MemBufferBase;

    /* Get the name of the file */    
    LFile_GetTempName(Object.FileBufferName);    

    /* Create the file */
    Object.FileBuffer = LFile_Open(Object.FileBufferName,LFile_READ_WRITE);

    if(Object.FileBuffer==NULL)
        Throw(LFile_CANT_OPEN_FILE);
        
    Object.FileBufferSize = 0;
    
    return LMemory_NewObject(LDiskStack,Object);
}


/* ---------------------------------------------------------------------------------
 *  Delete
 * ---------------------------------------------------------------------------------
 * Destructor */

void LDiskStack_Delete(LDiskStack** ThisA) {
    
    /* Deallocate the memory buffer */
    LMemory_Free(&(*ThisA)->MemBufferBase);
    
    /* Close thye file */
    LFile_Close(&(*ThisA)->FileBuffer);
    
    /* Delete the file */
    LFile_Remove((*ThisA)->FileBufferName);

    /* Deallocate object */
    LMemory_DeleteObject(ThisA);
}


/* ---------------------------------------------------------------------------------
 *  Push
 * ---------------------------------------------------------------------------------
 */

void LDiskStack_Push(LDiskStack* This, const void* inBlock, ui4 inSize) {

    ui4 theSizeUsed = (ui4)This->MemTop - (ui4)This->MemBufferBase;
    ui4 theSizeLeft = This->MemBufferSize - theSizeUsed;
        
    if (inSize >= theSizeLeft) {        
        /* Seek the end of the file */
        LFile_Seek(This->FileBuffer,This->FileBufferSize,LFile_START);
        
        /* Copy the used part of the buffer into the file */
        LFile_Write(This->FileBuffer,(ui1*)This->MemBufferBase,theSizeUsed);
        
        /* Update the stack pointer */
        This->MemTop = This->MemBufferBase;
        
        /* Copy the data into the file */
        LFile_Write(This->FileBuffer,(ui1*)inBlock,inSize);
        
        /* Update the size of the file */
        This->FileBufferSize = This->FileBufferSize + inSize + theSizeUsed;
    }
    else {
        /* Copy the left data into the memory buffer */
        LMemory_Copy((ui1*)inBlock,(ui1*)This->MemTop,inSize);
        
        /* Update the stack pointer */
        This->MemTop = (void*)((i1*)This->MemTop + inSize);
    }
}


/* ---------------------------------------------------------------------------------
 *  Pop
 * ---------------------------------------------------------------------------------
 */

void LDiskStack_Pop(LDiskStack* This, void* outBlock, const ui4 inSize) {

    ui4 theSizeUsed = (ui4)This->MemTop - (ui4)This->MemBufferBase;
    
    if(inSize > theSizeUsed) {
        
        /* Calculate the offset from the end of the file */
        ui4 theOffset = inSize - theSizeUsed;
                
        /* Control if the requested data exceed the dimension of the buffer */      
        if(theOffset > This->FileBufferSize)
            Throw(LMemory_OUT_OF_MEMORY);
        
        /* the new size of the file buffer */
        This->FileBufferSize = This->FileBufferSize - theOffset;
                
        /* Copy the data from the file into the memory block */
        LFile_Seek(This->FileBuffer,This->FileBufferSize,LFile_START);
        LFile_Read(This->FileBuffer,(ui1*)outBlock,theOffset);
        
        /* Copy the left data from the stack to the memory block */
        LMemory_Copy((ui1*)This->MemBufferBase,(ui1*)outBlock+theOffset,theSizeUsed);
        
        /* Update the stack pointer */
        This->MemTop = This->MemBufferBase;

    }
    else {
        /* Update the stack pointer */
        This->MemTop = (void*)((i1*)This->MemTop - inSize);
        
        /* Copy the left data into the memory buffer */
        LMemory_Copy((ui1*)This->MemTop,(ui1*)outBlock,inSize);     
    }
}


/* ---------------------------------------------------------------------------------
 *  IsEmpty
 * ---------------------------------------------------------------------------------
 */
 
Bool LDiskStack_IsEmpty(LDiskStack* This) {
    return ( (This->FileBufferSize==0) && (This->MemBufferBase==This->MemTop) );
}


/* ---------------------------------------------------------------------------------
 *  GetSize
 * ---------------------------------------------------------------------------------
 */

ui4 LDiskStack_GetSize(LDiskStack* This) {
    return This->FileBufferSize + (ui4)This->MemTop - (ui4)This->MemBufferBase;
}


/* Copyright (C) 2001 Camil Demetrescu, Irene Finocchi, Francesco Mungiguerra

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
