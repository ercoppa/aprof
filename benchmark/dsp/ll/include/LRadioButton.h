/* ============================================================================
 *  LRadioButton.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jun 5, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:24 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LRadioButton__
#define __LRadioButton__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"
    
/* defines... */
#define LRadioButton_MAX_TEXT 512
#define LRadioButton_ID 0x801C
#define LRadioButton_CHECKED   0
#define LRadioButton_UNCHECKED 1

/* exception codes */
enum { 
    LRadioButton_CANT_CREATE = LRadioButton_ID<<16
};

/* typedefs... */
typedef struct tagLRadioButton LRadioButton;

typedef void (*LRadioButton_THandler) (LRadioButton *inRadio);

typedef enum tagLRadioButton_TFont {
    LRadioButton_ANSIFIXED,
    LRadioButton_ANSIVAR,
    LRadioButton_SYSTEMFIXED,
    LRadioButton_SYSTEMVAR
} LRadioButton_TFont;

/* functions belonging to LRadioButton */
LRadioButton* LRadioButton_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, 
                                            i1* inText, LRadioButton_THandler inHandler);
void          LRadioButton_Destroy         (LRadioButton* inRadio);
void          LRadioButton_Move            (LRadioButton* inRadio, i4 inX, i4 inY);
void          LRadioButton_Resize          (LRadioButton* inRadio, ui4 inWidth, ui4 inHeight);
void          LRadioButton_SetFocus        (LRadioButton* inRadio);
void          LRadioButton_SetState        (LRadioButton* inRadio, ui2 inCheckedUnchecked);
ui2           LRadioButton_QueryState      (LRadioButton* inRadio);
void          LRadioButton_SetFont         (LRadioButton* inButton, LRadioButton_TFont inFont);
void          LRadioButton_Enable          (LRadioButton* inButton, Bool inEnabled);
LWindow*      LRadioButton_GetParentWindow (LRadioButton* inRadio);

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

