/* ============================================================================
 *  LMenu_win32.h
 * ============================================================================

 *  Author:         (c) 2003 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Feb 26, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:18 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LMenu_win32__
#define __LMenu_win32__

#if __LSL_OS_GUI__ == __LSL_Win32__

typedef struct tagLMenuItem {
	ui4 mItemID;
	ui2 mShortCut;
	void* mSubMenu; /* LMenu* */
} LMenuItem;

struct tagLMenu {
    HMENU mHmenu;
    i1* mMenuName;
	void (*mInitHandler) (void*);
	void* mParam;

	LArray* mAccelerators;
};

#endif
#endif

/* Copyright (C) 2003 Andrea Ribichini

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

