/* ============================================================================
 *  LTextEditor.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Mar 10, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:36 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LTextEditor__
#define __LTextEditor__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines... */
#define LTextEditor_ID 0x8015 

/* exception codes */
enum { 
    LTextEditor_CANT_CREATE = LTextEditor_ID<<16
};

/* macros */
#define LTextEditor_IsVirtualKey(x) ((x)&0xFFFF0000)

/* typedefs... */
typedef enum tagLTextEditor_TEventID {
    LTextEditor_MOUSE_LCLK    = 0x00,
    LTextEditor_MOUSE_LDBLCLK = 0x01,
    LTextEditor_MOUSE_RCLK    = 0x02,
    LTextEditor_KEYDOWN       = 0x03
} LTextEditor_TEventID;

typedef enum tagLTextEditor_TVirtKey { /* i4... */
	LTextEditor_UP            = 0xffff0000,
	LTextEditor_DOWN          = 0xffff0001,
	LTextEditor_LEFT          = 0xffff0002,
	LTextEditor_RIGHT         = 0xffff0003,
	LTextEditor_PAGEDOWN      = 0xffff0004,
	LTextEditor_PAGEUP        = 0xffff0005,
	LTextEditor_END           = 0xffff0006,
	LTextEditor_HOME          = 0xffff0007,
	LTextEditor_BACKSPACE     = 0xffff0008,
	LTextEditor_RETURN        = 0xffff0009,
	LTextEditor_DELETE        = 0xffff000a
} LTextEditor_TVirtKey;

typedef enum tagLTextEditor_TFont {
    LTextEditor_ANSIFIXED,
    LTextEditor_ANSIVAR,
    LTextEditor_SYSTEMFIXED,
    LTextEditor_SYSTEMVAR
} LTextEditor_TFont;

typedef enum tagLTextEditor_TStyle {
    LTextEditor_VScrollBar = 1,
    LTextEditor_HScrollBar = 2
} LTextEditor_TStyle;

typedef struct tagLTextEditor LTextEditor;
typedef Bool (*LTextEditor_THandler) (LTextEditor* inEditor, ...);

/*functions belonging to LTextEditor*/
LTextEditor* LTextEditor_Create (ui4 inStyle, LWindow* inWindow, ui4 inX, ui4 inY, ui4 inWidth, ui4 inHeight, Bool inFrameOn);
void LTextEditor_Destroy (LTextEditor* inEditor);
void LTextEditor_Move (LTextEditor* inEditor, ui4 inX, ui4 inY);
void LTextEditor_Resize (LTextEditor* inEditor, ui4 inWidth, ui4 inHeight);
void LTextEditor_SetTextBuffer (LTextEditor* inEditor, const i1* inBuffer, ui4 inSize);
ui4 LTextEditor_GetTextBufferSize (LTextEditor* inEditor);
void LTextEditor_GetTextBuffer (LTextEditor* inEditor, i1* outBuffer);
void LTextEditor_SetTextFont (LTextEditor* inEditor, LTextEditor_TFont inFont);
void LTextEditor_SetFocus (LTextEditor *inEditor);
/*functions formerly in LClipboard*/
void LTextEditor_Undo (LTextEditor* inEditor);
void LTextEditor_Cut (LTextEditor* inEditor);
void LTextEditor_Copy (LTextEditor* inEditor);
void LTextEditor_Paste (LTextEditor* inEditor);
void LTextEditor_SelectAll (LTextEditor* inEditor); 
Bool LTextEditor_CanUndo (LTextEditor* inEditor);
Bool LTextEditor_IsTextAvailable ();
Bool LTextEditor_IsSelectionAvailable (LTextEditor* inEditor);
/*NEW FUNCTIONS*/
void LTextEditor_GetSelection (LTextEditor* inEditor, ui4* outStart, ui4* outEnd);
void LTextEditor_SetSelection (LTextEditor* inEditor, ui4 inStart, ui4 inEnd);
void LTextEditor_ReplaceSelection (LTextEditor* inEditor, const i1* inBuffer, ui4 inSize);
void LTextEditor_DeleteSelection (LTextEditor* inEditor);
void LTextEditor_ScrollToCursor (LTextEditor* inEditor);
void LTextEditor_Scroll (LTextEditor* inEditor, i4 inCharHor, i4 inLinesVer);
ui4 LTextEditor_GetLineCount (LTextEditor* inEditor);
ui4 LTextEditor_GetLineIndex (LTextEditor* inEditor, ui4 inLine);
ui4 LTextEditor_GetLineLength (LTextEditor* inEditor, ui4 inIndex);
ui4 LTextEditor_GetLine (LTextEditor* inEditor, ui4 inLine, i1* outBuffer);
Bool LTextEditor_GetModify (LTextEditor* inEditor);
void LTextEditor_SetModify (LTextEditor* inEditor, Bool inFlag);
ui4 LTextEditor_GetFirstVisibleLine (LTextEditor* inEditor);
void LTextEditor_InstallHandler (LTextEditor* inEditor, LTextEditor_TEventID inEventID, Bool inLater, LTextEditor_THandler inHandler);
void LTextEditor_GetCursorPos (LTextEditor* inEditor, ui4* outLine, ui4* outCol);
LWindow* LTextEditor_GetParentWindow (LTextEditor* inEditor); /* [CD020718] */
void LTextEditor_SetSelectionColor (LTextEditor* inEditor, ui4 inColor);
void LTextEditor_CharFromPos (LTextEditor* inEditor, ui4 inX, ui4 inY, ui4* outCharIndex);
void LTextEditor_SetSelectionStatus (LTextEditor* inEditor, Bool inVisible);

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

