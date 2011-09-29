/* ============================================================================
 *  LApplication.h
 * ============================================================================

 *  Author:         (c) 2001 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 20, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:54 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LApplication__
#define __LApplication__

#include "LConfig.h"
#include "LType.h"
#include "LMemory.h"
#include "LFile.h"
#include "LTime.h"
#include "LThread.h"
#include "LSemaphore.h"
/*this way all the user has to include is LApplication.h*/
#include "LGlobals.h"
#include "LWindow.h"
#include "LMenu.h"
#include "LDialog.h"
#include "LTextEditor.h"
#include "LDraw.h"
#include "LBitmap.h"
#include "LPushButton.h"
#include "LCheckBox.h"
#include "LRadioButton.h"
#include "LGroupBox.h"
#include "LProgressBar.h"
#include "LStatusBar.h"
#include "LStaticText.h"
#include "LCursor.h"
#include "LListBox.h"
#include "LTextField.h"
#include "LScrollBar.h"
#include "LSizeBox.h"
#include "LFont.h"
#include "LToolbar.h"

/* defines... */
#define LApplication_MAX_TITLE  512
#define LApplication_ID         0x800F

/* typedefs... */
typedef enum tagLApplication_TLayout {
    LApplication_CASCADE,
    LApplication_TILE_VERTICALLY,
    LApplication_TILE_HORIZONTALLY
} LApplication_TLayout;

typedef void (*LApplication_THandler) ();
typedef void (*LApplication_TDropHandler) (i1* inBuffer, ui4 inX, ui4 inY);

/* functions belonging to LApplication */
void LApplication_Init                ();
void LApplication_SetTitle            (const i1* inTitle);
void LApplication_GetTitle            (i1* outTitle);
void LApplication_GetWorkspaceSize    (ui4* outWidth, ui4* outHeight);
void LApplication_Quit                ();
void LApplication_WindowLayout        (LApplication_TLayout inLayout);
void LApplication_SetTimer            (ui4 inTicks, LApplication_THandler inHandler);
i1*  LApplication_GetArgs             ();
void LApplication_SetDropHandler      (LApplication_TDropHandler inHandler);

Bool LApplication_GetFocus            (ui2* outObjID, void** outObj);
void LApplication_SetCheckBlocksCount (Bool inStatus);
void LApplication_GetSystemInfo       (i1* outBuffer, ui4 inBufSize);

void _LApplication_InitWindowManager  ();
void _LApplication_Run                ();

#endif

/* Copyright (C) 2001 Andrea Ribichini

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




