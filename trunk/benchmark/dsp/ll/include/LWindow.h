/* ============================================================================
 *  LWindow.h
 * ============================================================================

 *  Author:         (c) 2001-2003 Andrea Ribichini, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        Dec 20, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:42 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LWindow__
#define __LWindow__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LCursor.h"

/* defines... */
#define LWindow_MAX_TITLE 512
#define LWindow_ID 0x8010

/* exception codes */
enum { 
    LWindow_CANT_CREATE = LWindow_ID<<16
};

/* typedefs...*/
typedef enum tagLWindow_TEventID {
    LWindow_MOUSE_DOWN  = 0x00,
    LWindow_CLOSE       = 0x01,
    LWindow_ACTIVATE    = 0x02,
    LWindow_RESIZE      = 0x03,
    LWindow_MOUSE_MOVE  = 0x04,
    LWindow_SET_FOCUS   = 0x05
} LWindow_TEventID;

typedef enum tagLWindow_TSizeStatus {
    LWindow_USER,
    LWindow_MINIMIZED,
    LWindow_MAXIMIZED
} LWindow_TSizeStatus;

typedef enum tagLWindow_TWindowType {
    LWindow_REGULAR,
    LWindow_FLOATING
} LWindow_TWindowType;

typedef enum tagLWindow_TStyle {
    LWindow_Resizeable = 1,
    LWindow_Caption    = 2,
    LWindow_MinMaxBox  = 4
} LWindow_TStyle;

typedef struct tagLWindow LWindow;
typedef Bool (*LWindow_THandler) (LWindow* inWindow, ...);

/* functions belonging to LWindow */
LWindow* LWindow_Create (LWindow_TWindowType inWindowType, ui4 inStyle);
void     LWindow_Destroy (LWindow* inWindow);
void     LWindow_SetSizeStatus (LWindow* inWindow, LWindow_TSizeStatus inStatus);
LWindow_TSizeStatus LWindow_GetSizeStatus (LWindow* inWindow);
void     LWindow_Move (LWindow* inWindow, ui4 inX, ui4 inY);
void     LWindow_Resize (LWindow* inWindow, ui4 inWidth, ui4 inHeight);
void     LWindow_Finalize (LWindow* inWindow);
void     LWindow_SetTitle (LWindow* inWindow, const i1* inTitle);
void     LWindow_GetTitle (LWindow* inWindow, i1* inTitle);
void     LWindow_BringToFront (LWindow* inWindow);
void     LWindow_Close (LWindow* inWindow);
LWindow* LWindow_GetFrontWindow ();
void     LWindow_InstallEvent (LWindow* inWindow, LWindow_TEventID inEventID, LWindow_THandler inEventHandler); 
LWindow_TWindowType LWindow_GetType (LWindow* inWindow);
void     LWindow_GetClientAreaSize (LWindow* inWindow, ui4* outWidth, ui4* outHeight);
void     LWindow_GetPosition (LWindow* inWindow, ui4* outX, ui4* outY);
void     LWindow_SetUserData(LWindow* inWindow, const void* inData); /* [CD020718] */
void*    LWindow_GetUserData(LWindow* inWindow); /* [CD020718] */

void     LWindow_SetCursorShape (LWindow* inWindow, LCursor_TShape inCursorShape);
LCursor_TShape LWindow_GetCursorShape (LWindow* inWindow);

void     LWindow_EnableMinMax (LWindow* inWindow, ui4 inMinX, ui4 inMinY, ui4 inMaxX, ui4 inMaxY);
void     LWindow_DisableMinMax (LWindow* inWindow);

#endif

/* Copyright (C) 2001-2003 Andrea Ribichini, Camil Demetrescu

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

