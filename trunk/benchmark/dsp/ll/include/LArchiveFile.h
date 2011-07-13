/* ============================================================================
 *  LArchiveFile.h
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu, Francesco Mungiguerra. 
 *  License:        See the end of this file for license information
 *  Created:        November 28, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:55 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LArchiveFile__
#define __LArchiveFile__

#include "LType.h"
#include "LArray.h"
#include "LFile.h"
#include "LXPBlock.h"

/* COMPONENT ID */
#define LArchiveFile_ID 0x8001

typedef struct LArchiveFile LArchiveFile;

enum { 
    LArchiveFile_MAGIC_NUMBER   = 0x4C4E4456,  /* 'LNDV' */
    LArchiveFile_VERSION        = 0x0000 
};

typedef enum {
    LArchiveFile_READ       = 0x01,
    LArchiveFile_WRITE      = 0x02,
    LArchiveFile_READ_WRITE = LArchiveFile_READ | LArchiveFile_WRITE
} LArchiveFile_TOpenMode;
    
enum {
    LArchiveFile_ILLEGAL_FILE_TYPE = LArchiveFile_ID<<16, 
    LArchiveFile_CANT_OPEN_FILE, 
    LArchiveFile_DAMAGED_FILE, 
    LArchiveFile_NEWER_FILE_VERSION, 
    LArchiveFile_IO_ERROR, 
    LArchiveFile_OUT_OF_RANGE, 
    LArchiveFile_READ_ONLY_ACCESS, 
    LArchiveFile_WRITE_ONLY_ACCESS
};

LArchiveFile*   LArchiveFile_Open           (const i1* inFileName, LArchiveFile_TOpenMode inMode);
void            LArchiveFile_Close          (LArchiveFile** ThisA);

ui4             LArchiveFile_AddBlock       (LArchiveFile* This, ui4 inBlockTag, LXPBlock* inTagBlock);
void            LArchiveFile_RemoveBlock    (LArchiveFile* This, ui4 inBlockIdx); 

ui4             LArchiveFile_GetBlockTag    (LArchiveFile* This, ui4 inBlockIdx);
LXPBlock*       LArchiveFile_GetXPBlock     (LArchiveFile* This, ui4 inBlockIdx);
ui4             LArchiveFile_GetBlocksCount (LArchiveFile* This);
 
#endif


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
