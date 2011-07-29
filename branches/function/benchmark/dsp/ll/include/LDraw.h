/* ============================================================================
 *  LDraw.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Apr 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:05 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDraw__
#define __LDraw__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LException.h"
#include "LWindow.h"
#include "LBitmap.h"
#include "LCursor.h"
#include "LFont.h"

/* typedefs... */

typedef enum tagLDraw_TBufferType {
    LDraw_LOCAL,
    LDraw_GLOBAL
} LDraw_TBufferType;

typedef enum tagLDraw_TBGTextMode {
    LDraw_OPAQUE,
    LDraw_TRANSPARENT
} LDraw_TBGTextMode;

typedef enum tagLDraw_TEventID {
    LDraw_MOUSE_DOWN  = 0x00,
    LDraw_REFRESH     = 0x01,
    LDraw_MOUSE_MOVE  = 0x02
} LDraw_TEventID;

typedef struct tagLDraw LDraw;
typedef Bool (*LDraw_THandler) (LDraw* inDraw, ...);

/* defines... */
#define LDraw_ID 0x801A

/* exception codes */
enum { 
    LDraw_CANT_CREATE = LDraw_ID<<16
};

/* functions belonging to LDraw */
LDraw*         LDraw_CreateLocal           (LWindow* inWindow, ui4 inX, ui4 inY, ui4 inDX, ui4 inDY, 
                                            ui4 inBDX, ui4 inBDY, Bool inFrameOn);
LDraw*         LDraw_CreateGlobal          (LWindow* inWindow, ui4 inX, ui4 inY, ui4 inDX, ui4 inDY, Bool inFrameOn);
void           LDraw_Destroy               (LDraw* inDraw);
void           LDraw_Resize                (LDraw* inDraw, ui4 inWidth, ui4 inHeight);
void           LDraw_Move                  (LDraw* inDraw, ui4 inX, ui4 inY);
void           LDraw_InstallEvent          (LDraw* inDraw, LDraw_TEventID inEventID, LDraw_THandler inEventHandler); 
void           LDraw_Update                (LDraw* inDraw);
void           LDraw_SaturateBuffer        (LDraw* inDraw);
void           LDraw_MoveOrigin            (LDraw* inDraw, i4 inX, i4 inY);

void           LDraw_SetPixel              (LDraw* inDraw, i4 inX, i4 inY, ui4 inColor);
ui4            LDraw_GetPixel              (LDraw* inDraw, i4 inX, i4 inY);
void           LDraw_MoveTo                (LDraw* inDraw, i4 inX, i4 inY);
void           LDraw_LineTo                (LDraw* inDraw, i4 inX, i4 inY);
void           LDraw_GetCurrentPosition    (LDraw* inDraw, i4* outX, i4* outY);
void           LDraw_Rectangle             (LDraw* inDraw, i4 inTopX, i4 inTopY, i4 inBottomX, i4 inBottomY);
void           LDraw_Ellipse               (LDraw* inDraw, i4 inTopX, i4 inTopY, i4 inBottomX, i4 inBottomY);
void           LDraw_Polygon               (LDraw* inDraw, i4* inVertices, ui4 inNumVertices);
void           LDraw_SetPen                (LDraw* inDraw, ui4 inColor, ui4 inWidth);
void           LDraw_SetBrush              (LDraw* inDraw, ui4 inColor);
void           LDraw_SetPenToTransparent   (LDraw* inDraw);
void           LDraw_SetBrushToTransparent (LDraw* inDraw);

void           LDraw_BlitBitmap            (LDraw* inDraw, LBitmap* inBitmap, i4 inX, i4 inY);
LBitmap*       LDraw_GetBitmap             (LDraw* inDraw, i4 inX, i4 inY, ui4 inWidth, ui4 inHeight);

void           LDraw_DrawText              (LDraw* inDraw, i4 inX, i4 inY, i1* inString, ui4 inLength);
void           LDraw_SetTextColor          (LDraw* inDraw, ui4 inColor);
void           LDraw_SetBGTextColor        (LDraw* inDraw, ui4 inColor);
void           LDraw_SetBGTextMode         (LDraw* inDraw, LDraw_TBGTextMode inMode);
ui4            LDraw_GetLineSpacing        (LDraw* inDraw);
ui4            LDraw_GetStringPixLength    (LDraw* inDraw, i1* inString, ui4 inLength);
void           LDraw_SetTextFont           (LDraw* inDraw, LFont inFont);
LWindow*       LDraw_GetParentWindow       (LDraw* inDraw);

void           LDraw_SetCursorShape        (LDraw* inDraw, LCursor_TShape inCursorShape);
LCursor_TShape LDraw_GetCursorShape        (LDraw* inDraw);

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

