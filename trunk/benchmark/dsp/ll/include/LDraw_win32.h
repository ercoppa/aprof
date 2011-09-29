/* ============================================================================
 *  LDraw_win32.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Apr 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:06 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDraw_win32__
#define __LDraw_win32__

#if __LSL_OS_GUI__ == __LSL_Win32__

struct tagLDraw {
    HWND mHwndDraw;
    void (*mHandler) ();
    ui2 mID;
    LWindow* mParentWindow;

    LDraw_TBufferType mType;
    HDC mMemHdc;
    HPEN mHpen;
    HBRUSH mHbrush;
    int mmaxX;
    int mmaxY;
    Bool (*mMouseDownHandler) ();
    Bool (*mMouseMoveHandler) ();
    Bool (*mRefreshHandler) ();
    HBITMAP mLocalBuffer;
    i4 Ox;
    i4 Oy;

	LCursor_TShape mCursorShape;

	HFONT mFont;
};

typedef Bool (*_LDraw_THandler) ();

#endif
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

