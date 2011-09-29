/* ============================================================================
 *  LTextField.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jul 29, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:37 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LTextField__
#define __LTextField__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"

/* defines...*/
#define LTextField_ID 0x8022

/* exception codes */
enum { 
    LTextField_CANT_CREATE = LTextField_ID<<16
};

/* typedefs... */
typedef enum tagLTextField_TFont {
    LTextField_ANSIFIXED,
    LTextField_ANSIVAR,
    LTextField_SYSTEMFIXED,
    LTextField_SYSTEMVAR
} LTextField_TFont;

/*typedef enum tagLTextField_TEditMode {
    LTextField_STANDARD
} LTextField_TEditMode;*/

typedef struct tagLTextField LTextField;
typedef Bool (*LTextField_THandler) (LTextField* inEditor);

/* functions belonging to LTextField */
LTextField* LTextField_Create (LWindow* inWindow, ui4 inX, ui4 inY, ui4 inWidth, ui4 inHeight, Bool inFrameOn);
void LTextField_Destroy (LTextField* inEditor);
void LTextField_Move (LTextField* inEditor, ui4 inX, ui4 inY);
void LTextField_Resize (LTextField* inEditor, ui4 inWidth, ui4 inHeight);
void LTextField_SetTextBuffer (LTextField* inEditor, i1* inBuffer);
i4 LTextField_GetTextBufferSize (LTextField* inEditor);
void LTextField_GetTextBuffer (LTextField* inEditor, i1* outBuffer, i4 inLength);
void LTextField_SetFont (LTextField* inEditor, LTextField_TFont inFont);
void LTextField_SetFocus (LTextField *inEditor);
LTextField* LTextField_GetFocus ();
/*functions formerly in LClipboard*/
void LTextField_Undo (LTextField* inEditor);
void LTextField_Cut (LTextField* inEditor);
void LTextField_Copy (LTextField* inEditor);
void LTextField_Paste (LTextField* inEditor);
void LTextField_SelectAll (LTextField* inEditor); 
Bool LTextField_CanUndo (LTextField* inEditor);
Bool LTextField_IsTextAvailable ();
Bool LTextField_IsSelectionAvailable (LTextField* inEditor);
/*NEW FUNCTIONS*/
/*void LTextEditor_GetSelection (LTextEditor* inEditor, i4* outStart, i4* outEnd);
void LTextEditor_SetSelection (LTextEditor* inEditor, i4 inStart, i4 inEnd);
void LTextEditor_ReplaceSelection (LTextEditor* inEditor, i1* inString);
void LTextEditor_DeleteSelection (LTextEditor* inEditor);
void LTextEditor_ScrollToCursor (LTextEditor* inEditor);
void LTextEditor_Scroll (LTextEditor* inEditor, i4 inCharHor, i4 inLinesVer);
i4 LTextEditor_GetLineCount (LTextEditor* inEditor);
i4 LTextEditor_GetLineIndex (LTextEditor* inEditor, i4 inLine);
i4 LTextEditor_GetLineLength (LTextEditor* inEditor, i4 inIndex);
i4 LTextEditor_GetLine (LTextEditor* inEditor, i4 inLine, i1* inBuffer);*/
/*New New Functions*/
/*Bool LTextEditor_GetModify (LTextEditor* inEditor);
void LTextEditor_SetModify (LTextEditor* inEditor, Bool inFlag);
i4 LTextEditor_GetFirstVisibleLine (LTextEditor* inEditor);*/
void LTextField_InstallReturnHandler (LTextField* inEditor, LTextField_THandler inHandler);
/*i4 LTextEditor_GetCursorLine (LTextEditor* inEditor);*/
LWindow* LTextField_GetParentWindow (LTextField* inEditor);
void LTextField_Enable (LTextField* inEditor, Bool inEnabled);

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

