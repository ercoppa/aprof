/* ============================================================================
 *  LFile.h
 * ============================================================================

 *  Author:         (c) 2001-2002 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:08 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LFile__
#define __LFile__

#include "LType.h"
#include "LConfig.h"
#include "LXPBlock.h"
#include "LTime.h"

/* COMPONENT ID */
#define LFile_ID    0x8006

typedef struct LFile LFile;

typedef struct LFile_SearchHandle LFile_SearchHandle;

typedef struct tagLFile_Info {
	Bool  mIsDir;
	LTime mCreated;
	LTime mLastMod;
} LFile_Info;
				
/*end of RIBBI*/

enum {
    LFile_CANT_OPEN_FILE = LFile_ID<<16, 
    LFile_IO_ERROR, 
    LFile_CANT_REMOVE_FILE,
    LFile_CANT_RENAME_FILE
};

typedef enum {
    LFile_READ       = 0x01,
    LFile_WRITE      = 0x02,
    LFile_READ_WRITE = LFile_READ | LFile_WRITE
} LFile_TOpenMode;

typedef enum {
    LFile_START      = 0x01,
    LFile_CURR       = 0x02,
    LFile_END        = 0x03
} LFile_TSeekMode;

enum { 
    LFile_MAX_PATHNAME_LEN = 1024 
};

LFile*              LFile_Open          (const i1* inFileName, LFile_TOpenMode inMode);
void                LFile_Close         (LFile** ThisA);

void                LFile_Write         (LFile* This, const void* inData, ui4 inSize);
ui4                 LFile_Read          (LFile* This, void* outData, ui4 inSize);

void                LFile_WriteXPBlock  (LFile* This, LXPBlock* inBlock);
LXPBlock*           LFile_ReadXPBlock   (LFile* This, ui4 inSize);

void                LFile_WriteN1       (LFile* This, const void* inVal);
void                LFile_WriteN2       (LFile* This, const void* inVal);
void                LFile_WriteN4       (LFile* This, const void* inVal);
void                LFile_WriteN8       (LFile* This, const void* inVal);

void                LFile_ReadN1        (LFile* This, void* outVal);
void                LFile_ReadN2        (LFile* This, void* outVal);
void                LFile_ReadN4        (LFile* This, void* outVal);
void                LFile_ReadN8        (LFile* This, void* outVal);

void                LFile_WriteString   (LFile* This, i1* inBuf);
ui4                 LFile_ReadString    (LFile* This, i1* outBuf, ui4 inBufSize, i1* inTerminator);

Bool                LFile_Seek          (LFile* This, i4 inOffset, LFile_TSeekMode inMode);
ui4                 LFile_Tell          (LFile* This);
ui4                 LFile_GetSize       (LFile* This);

Bool                LFile_Exists        (const i1* inFileName);
void                LFile_Remove        (const i1* inFileName);
void                LFile_Rename        (const i1* inOldFileName, const i1* inNewFileName);
void                LFile_GetTempName   (i1 outFileName[LFile_MAX_PATHNAME_LEN]);

void                LFile_CutPath       (i1 thruPathName[LFile_MAX_PATHNAME_LEN]);
void                LFile_CutName       (i1 thruPathName[LFile_MAX_PATHNAME_LEN]);
void                LFile_CutPathNameExt(i1 thruPathName[LFile_MAX_PATHNAME_LEN]);

LFile_SearchHandle* LFile_FindFirst     (const i1* inSearchPath, i1* outFileName);
Bool                LFile_FindNext      (LFile_SearchHandle* inHandle, i1* outFileName);
void                LFile_FindClose     (LFile_SearchHandle** ThisA);

Bool                LFile_GetFileInfo   (const i1* inFileName, LFile_Info* outInfo);

Bool                LFile_CreateDir     (const i1* inCompletePath);
Bool                LFile_RemoveDir     (const i1* inCompletePath);

/*end of RIBBI*/
 
#endif


/* Copyright (C) 2001 Camil Demetrescu, Andrea Ribichini

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
