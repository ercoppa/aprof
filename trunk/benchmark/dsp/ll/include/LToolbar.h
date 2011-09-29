/* ============================================================================
 *  LToolbar.h
 * ============================================================================

 *  Author:         (c) 2003 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Apr 27, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:40 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LToolbar__
#define __LToolbar__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"
#include "LBitmap.h"

/* defines... */
#define LToolbar_ID 0x8033
#define LToolbar_MAX_TOOLTIP_TEXT 128

/* typedefs... */
typedef struct tagLToolbar LToolbar;

typedef enum tagLToolbar_TButtonStyle {
	LToolbar_BUTTON,
	LToolbar_SEPARATOR
} LToolbar_TButtonStyle;

typedef enum tagLToolbar_TButtonState {
	LToolbar_ENABLED,
	LToolbar_DISABLED
} LToolbar_TButtonState;

typedef struct tagLToolbar_TButton {
	ui2 mBitmapIndex;
	ui4 mItemID;
	LToolbar_TButtonState mState;
	LToolbar_TButtonStyle mStyle;
	i1 mTooltipText[LToolbar_MAX_TOOLTIP_TEXT];
} LToolbar_TButton;

/* exception codes */
enum { 
    LToolbar_CANT_CREATE = LToolbar_ID<<16
};

/* functions belonging to LToolbar */
LToolbar* LToolbar_Create         (ui2 inNumButtons, LBitmap* inBitmap, LToolbar_TButton* inButtonArray);
void      LToolbar_Destroy        (LToolbar* inToolbar);
void      LToolbar_SetButtonState (LToolbar* inToolbar, ui4 inItemID, Bool inEnabled);

#endif

/* Copyright (C) 2003 Andrea Ribichini

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

