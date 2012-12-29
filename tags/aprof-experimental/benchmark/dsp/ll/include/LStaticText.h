/* ============================================================================
 *  LStaticText.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jul 21, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:33 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LStaticText__
#define __LStaticText__

/* includes...*/
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines... */
#define LStaticText_MAX_TEXT 512
#define LStaticText_ID 0x8020

/* exception codes */
enum { 
    LStaticText_CANT_CREATE = LStaticText_ID<<16
};

/* typedefs... */
typedef struct tagLStaticText LStaticText;

typedef enum tagLStaticText_TAlign {
    LStaticText_CENTER,
    LStaticText_LEFT,
    LStaticText_RIGHT
} LStaticText_TAlign;

typedef enum tagLStaticText_TFont {
    LStaticText_ANSIFIXED,
    LStaticText_ANSIVAR,
    LStaticText_SYSTEMFIXED,
    LStaticText_SYSTEMVAR
} LStaticText_TFont;

/* functions belonging to LStaticText */
LStaticText* LStaticText_Create          (LWindow* inWindow, i1* inString, LStaticText_TAlign inMode, 
                                          ui4 inX, ui4 inY, ui4 inDX, ui4 inDY);
void         LStaticText_Destroy         (LStaticText* inText);
void         LStaticText_Move            (LStaticText* inText, i4 inX, i4 inY);
void         LStaticText_Resize          (LStaticText* inText, ui4 inWidth, ui4 inHeight);
void         LStaticText_SetFont         (LStaticText* inText, LStaticText_TFont inFont);
LWindow*     LStaticText_GetParentWindow (LStaticText* inText);

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

