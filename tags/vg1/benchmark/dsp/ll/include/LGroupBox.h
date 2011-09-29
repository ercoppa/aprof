/* ============================================================================
 *  LGroupBox.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jun 6, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:12 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LGroupBox__
#define __LGroupBox__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* typedefs... */
typedef struct tagLGroupBox LGroupBox;

typedef enum tagLGroupBox_TFont {
    LGroupBox_ANSIFIXED,
    LGroupBox_ANSIVAR,
    LGroupBox_SYSTEMFIXED,
    LGroupBox_SYSTEMVAR
} LGroupBox_TFont;
    
/*defines...*/
#define LGroupBox_MAX_TEXT 512
#define LGroupBox_ID 0x801B

/* exception codes */
enum { 
    LGroupBox_CANT_CREATE = LGroupBox_ID<<16
};

/* functions belonging to LGroupBox */
LGroupBox* LGroupBox_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, i1* inText);
void       LGroupBox_Destroy         (LGroupBox* inGroup);
void       LGroupBox_Move            (LGroupBox* inGroup, i4 inX, i4 inY);
void       LGroupBox_Resize          (LGroupBox* inGroup, ui4 inWidth, ui4 inHeight);
void       LGroupBox_SetFont         (LGroupBox* inGroup, LGroupBox_TFont inFont);
LWindow*   LGroupBox_GetParentWindow (LGroupBox* inGroup);

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

