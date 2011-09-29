/* ============================================================================
 *  LScrollBar.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Aug 26, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:28 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LScrollBar__
#define __LScrollBar__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines... */
#define LScrollBar_ID 0x8023
#define LScrollBar_VER 0
#define LScrollBar_HOR 1

/* exception codes */
enum { 
    LScrollBar_CANT_CREATE = LScrollBar_ID<<16
};

/* typedefs... */
typedef struct tagLScrollBar LScrollBar;

typedef enum tagLScrollBar_TEvent {
    LScrollBar_LINEUP,
    LScrollBar_LINEDOWN,
    LScrollBar_PAGEUP,
    LScrollBar_PAGEDOWN,
    LScrollBar_THUMBPOSITION,
    LScrollBar_THUMBTRACK
} LScrollBar_TEvent;

typedef void (*LScrollBar_THandler) (LScrollBar* inBar, LScrollBar_TEvent inEvent, ui2 inInfo);

/* functions belonging to LScrollBar */
LScrollBar* LScrollBar_Create          (LWindow* inWindow, i4 inX, i4 inY, ui4 inDX, ui4 inDY, 
                                        ui2 inHorVer, LScrollBar_THandler inHandler);
void        LScrollBar_Destroy         (LScrollBar* inBar);
void        LScrollBar_Move            (LScrollBar* inBar, i4 inX, i4 inY);
void        LScrollBar_Resize          (LScrollBar* inBar, ui4 inDX, ui4 inDY);
void        LScrollBar_SetPosition     (LScrollBar* inBar, ui2 inPos);
ui2         LScrollBar_GetPosition     (LScrollBar* inBar);
void        LScrollBar_SetRange        (LScrollBar* inBar, ui2 inMinRange, ui2 inMaxRange);
LWindow*    LScrollBar_GetParentWindow (LScrollBar* inBar);

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

