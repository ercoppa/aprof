/* ============================================================================
 *  LBitmap.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        May 17, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:57 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LBitmap__
#define __LBitmap__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"

/* typedefs... */
typedef struct tagLBitmap LBitmap;

/* defines... */
#define LBitmap_ID 0x8018

/* exception codes */
enum { 
    LBitmap_CANT_CREATE = LBitmap_ID<<16
};

/* functions belonging to LBitmap */
LBitmap* LBitmap_CreateFromBMP (void* inBMPFileImage, ui4 inFileImageSize);
void     LBitmap_Destroy       (LBitmap* inBitmap);
void*    LBitmap_ConvertToBMP  (LBitmap* inBitmap, ui4* outSize);

#endif

/* Copyright (C) 2002 Andrea Ribichini

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

