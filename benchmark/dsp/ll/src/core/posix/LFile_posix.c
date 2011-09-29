/* ============================================================================
 *  LFile_posix.c
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LSL

 *  Last changed:   $Date: 2010/04/23 16:05:59 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LConfig.h"

#if __LSL_OS_CORE__ == __LSL_POSIX__ || __LSL_OS_CORE__ == __LSL_WIN32__

#include <stdio.h>
#include "LFile.h"
#include "LException.h"
#include "LMemory.h"
#include "LDebug.h"


/* MEMBER VARIABLES */
struct LFile {
    FILE* File;
};

/* MACROS */
#define OpenMode_(x) ((x==LFile_READ)? "rb" : (x==LFile_WRITE)? "wb" : "rb+")
#define SeekMode_(x) ((x==LFile_START)? SEEK_SET : (x==LFile_CURR)? SEEK_CUR : SEEK_END)


/* ----------------------------------------------------------------------------
 *  Open
 * ----------------------------------------------------------------------------
*/
LFile* LFile_Open(const i1* inFileName, LFile_TOpenMode inMode){

    LFile theObject;

    /* Create file if READ_WRITE mode and file does not exist */
    if (inMode==LFile_READ_WRITE) {
        if (!LFile_Exists(inFileName)){
            FILE* theTempFile;
            theTempFile = fopen(inFileName,"wb");
            if (theTempFile==NULL ) Throw(LFile_CANT_OPEN_FILE);
            fclose(theTempFile);
        }
    }

    /* Open file */
    theObject.File = fopen(inFileName, OpenMode_(inMode));
    if (theObject.File==NULL) Throw(LFile_CANT_OPEN_FILE);

    return (LFile*)LMemory_NewObject(LFile,theObject);
}


/* ----------------------------------------------------------------------------
 *  Close
 * ----------------------------------------------------------------------------
*/
void LFile_Close(LFile** ThisA){
    fclose((*ThisA)->File);
    LMemory_DeleteObject(ThisA);
}


/* ----------------------------------------------------------------------------
 *  Write
 * ----------------------------------------------------------------------------
*/
void LFile_Write(LFile* This, const void* inData, ui4 inSize){
    if (fwrite(inData,1,inSize,This->File)!=inSize) Throw(LFile_IO_ERROR);
}


/* ----------------------------------------------------------------------------
 *  Read
 * ----------------------------------------------------------------------------
*/
ui4 LFile_Read(LFile* This, void* outData, ui4 inSize){
    return (ui4)fread(outData,1,inSize,This->File);
}

    
/* ----------------------------------------------------------------------------
 *  Seek
 * ----------------------------------------------------------------------------
*/
Bool LFile_Seek(LFile* This, i4 inOffset, LFile_TSeekMode inMode){
    return (Bool)!fseek(This->File,inOffset,SeekMode_(inMode));
}


/* ----------------------------------------------------------------------------
 *  Tell
 * ----------------------------------------------------------------------------
*/
ui4 LFile_Tell(LFile* This){
    long int theOffset=ftell(This->File);
    if (theOffset==-1L) Throw(LFile_IO_ERROR);
    return (ui4)theOffset;
}


/* ----------------------------------------------------------------------------
 *  GetSize
 * ----------------------------------------------------------------------------
*/
ui4 LFile_GetSize(LFile* This){
    ui4 theFileSize;
    ui4 theCurrPos = ftell(This->File);
    fseek(This->File,0,SEEK_END);
    theFileSize = ftell(This->File);
    fseek(This->File,theCurrPos,SEEK_SET);
    return theFileSize;
}


/* ----------------------------------------------------------------------------
 *  Exists
 * ----------------------------------------------------------------------------
*/
Bool LFile_Exists(const i1* inFileName){
    FILE* theFile = fopen(inFileName,"rb");
    if (theFile==NULL) return FALSE;
    fclose(theFile);
    return TRUE;
}


/* ----------------------------------------------------------------------------
 *  Remove
 * ----------------------------------------------------------------------------
*/
void LFile_Remove(const i1* inFileName){
	#ifndef LEONARDO
    if (remove(inFileName)) Throw(LFile_CANT_REMOVE_FILE);
	#endif
}


/* ----------------------------------------------------------------------------
 *  Rename
 * ----------------------------------------------------------------------------
*/
void LFile_Rename(const i1* inOldFileName, const i1* inNewFileName){
	#ifndef LEONARDO
    if (rename(inOldFileName,inNewFileName)) Throw(LFile_CANT_RENAME_FILE);
	#endif
}


/* ----------------------------------------------------------------------------
 *  GetTempName
 * ----------------------------------------------------------------------------
*/
void LFile_GetTempName(i1 outFileName[LFile_MAX_PATHNAME_LEN]){
	#ifndef LEONARDO
    if (L_tmpnam>LFile_MAX_PATHNAME_LEN) Throw(LDebug_INTERNAL_ERROR);
    tmpnam(outFileName);
	#endif
}

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
