/* ============================================================================
 *  LWindow_win32.h
 * ============================================================================

 *  Author:         (c) 2001 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 20, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:43 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LWindow_win32__
#define __LWindow_win32__

#if __LSL_OS_GUI__ == __LSL_Win32__

struct tagLWindow {
    HWND mHwnd;
	void (*mHandler) (); /*to fix a bug...*/
    ui2 mID;
    LWindow_TWindowType mType;
    Bool (*mCloseHandler) (); 
    Bool (*mMouseDownHandler) ();
    Bool (*mMouseMoveHandler) ();
    Bool (*mActivateHandler) ();
    Bool (*mResizeHandler) ();
    Bool (*mSetFocusHandler) ();
    void* mUserData;

    void* mWidgetPointer; /*pointer to last widget to request focus...*/
    ui2 mWidgetType; /*...and its type*/

    HWND mStatusBarHandle;

	LCursor_TShape mCursorShape;

	Bool mMinMaxEnable;
	ui4 mMinX;
	ui4 mMinY;
	ui4 mMaxX;
	ui4 mMaxY;
};

typedef Bool (*_LWindow_THandler) ();

#endif
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

