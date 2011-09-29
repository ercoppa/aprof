/* ============================================================================
 *  LPushButton.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        May 19, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:22 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LPushButton__
#define __LPushButton__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"
#include "LBitmap.h"

/* defines... */
#define LPushButton_MAX_TEXT 512
#define LPushButton_ID 0x8017

/* exception codes */
enum { 
    LPushButton_CANT_CREATE = LPushButton_ID<<16
};

/* typedefs... */
typedef struct tagLPushButton LPushButton;

typedef void (*LPushButton_THandler) (LPushButton *inButton);

typedef enum tagLPushButton_TFont {
    LPushButton_ANSIFIXED,
    LPushButton_ANSIVAR,
    LPushButton_SYSTEMFIXED,
    LPushButton_SYSTEMVAR
} LPushButton_TFont;

/* functions belonging to LPushButton */
LPushButton* LPushButton_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, 
                                          i1* inText, LBitmap* inBitmap, LPushButton_THandler inHandler);
void         LPushButton_Destroy         (LPushButton* inButton);
void         LPushButton_Move            (LPushButton* inButton, i4 inX, i4 inY);
void         LPushButton_Resize          (LPushButton* inButton, ui4 inWidth, ui4 inHeight);
void         LPushButton_SetFocus        (LPushButton* inButton);
void         LPushButton_SetFont         (LPushButton* inButton, LPushButton_TFont inFont);
void         LPushButton_Enable          (LPushButton* inButton, Bool inEnabled);
LWindow*     LPushButton_GetParentWindow (LPushButton* inButton);

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

