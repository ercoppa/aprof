/* ============================================================================
 *  LListBox.h
 * ============================================================================

 *  Author:         (c) 2002-2003 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jul 24, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:14 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LListBox__
#define __LListBox__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines... */
#define LListBox_MAX_TEXT 512
#define LListBox_ID 0x801E

/* exception codes */
enum { 
    LListBox_CANT_CREATE = LListBox_ID<<16
};

/* typedefs... */
typedef struct tagLListBox LListBox;

typedef void (*LListBox_THandler) (LListBox* inLBox);

typedef enum tagLListBox_TFont {
    LListBox_ANSIFIXED,
    LListBox_ANSIVAR,
    LListBox_SYSTEMFIXED,
    LListBox_SYSTEMVAR
} LListBox_TFont;


/* functions belonging to LListBox */
LListBox* LListBox_Create              (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, 
                                        LListBox_THandler inDBLCLKHandler);
void      LListBox_Destroy             (LListBox* inLBox);
void      LListBox_Move                (LListBox* inLBox, i4 inX, i4 inY);
void      LListBox_Resize              (LListBox* inLBox, ui4 inWidth, ui4 inHeight);
void      LListBox_SetFocus            (LListBox* inBox);
void      LListBox_SetFont             (LListBox* inLBox, LListBox_TFont inFont);
void      LListBox_AddItem             (LListBox* inLBox, i1* inString);
void      LListBox_RemoveItem          (LListBox* inLBox, i1* inString);
Bool      LListBox_GetCurrentSelection (LListBox* inLBox, i1* outString);
void      LListBox_SetCurrentSelection (LListBox* inLBox, i1* inString);
LWindow*  LListBox_GetParentWindow     (LListBox* inLBox);

#endif

/* Copyright (C) 2002-2003 Andrea Ribichini

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

