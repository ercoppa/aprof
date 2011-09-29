/* ============================================================================
 *  LCheckBox.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        May 28, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:59 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LCheckBox__
#define __LCheckBox__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* typedefs... */
typedef struct tagLCheckBox LCheckBox;

typedef void (*LCheckBox_THandler) (LCheckBox *inBox);

typedef enum tagLCheckBox_TFont {
    LCheckBox_ANSIFIXED,
    LCheckBox_ANSIVAR,
    LCheckBox_SYSTEMFIXED,
    LCheckBox_SYSTEMVAR
} LCheckBox_TFont;

/* defines... */
#define LCheckBox_MAX_TEXT 512
#define LCheckBox_ID 0x8019
#define LCheckBox_CHECKED   0
#define LCheckBox_UNCHECKED 1
#define LCheckBox_INDETERMINATE 2

/* exception codes */
enum { 
    LCheckBox_CANT_CREATE = LCheckBox_ID<<16
};

/* functions belonging to LCheckBox */
LCheckBox* LCheckBox_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, 
                                      i1* inText, LCheckBox_THandler inHandler);
void       LCheckBox_Destroy         (LCheckBox* inBox);
void       LCheckBox_Move            (LCheckBox* inBox, i4 inX, i4 inY);
void       LCheckBox_Resize          (LCheckBox* inBox, ui4 inWidth, ui4 inHeight);
void       LCheckBox_SetFocus        (LCheckBox* inBox);
void       LCheckBox_SetState        (LCheckBox* inBox, ui2 inState);
ui2        LCheckBox_QueryState      (LCheckBox* inBox);
void       LCheckBox_SetFont         (LCheckBox* inBox, LCheckBox_TFont inFont);
void       LCheckBox_Enable          (LCheckBox* inBox, Bool inEnabled);
LWindow*   LCheckBox_GetParentWindow (LCheckBox* inBox);

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

