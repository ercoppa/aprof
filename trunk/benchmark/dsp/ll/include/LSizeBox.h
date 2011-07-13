/* ============================================================================
 *  LSizeBox.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Aug 29, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:32 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LSizeBox__
#define __LSizeBox__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines... */
#define LSizeBox_ID 0x8024

/* exception codes */
enum { 
    LSizeBox_CANT_CREATE = LSizeBox_ID<<16
};

/* typedefs... */
typedef struct tagLSizeBox LSizeBox;

/* functions belonging to LSizeBox */
LSizeBox* LSizeBox_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY);
void      LSizeBox_Destroy         (LSizeBox* inBox);
void      LSizeBox_Move            (LSizeBox* inBox, i4 inX, i4 inY);
void      LSizeBox_Resize          (LSizeBox* inBox, ui4 inDX, ui4 inDY);
LWindow*  LSizeBox_GetParentWindow (LSizeBox* inBox);

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

