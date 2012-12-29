/* ===========================================================================
 *  LGlobals_win32.h
 * ===========================================================================

 *  Author:         (c) 2002 Andrea Ribichini, Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        Jul 16, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:10 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LGlobals_win32__
#define __LGlobals_win32__

#if __LSL_OS_GUI__ == __LSL_WIN32__

/*these includes have global relevance...*/
#include <windows.h>
#include <commctrl.h> /*progress bar, status bar*/
#include <commdlg.h> /*color picker*/
#include <shellapi.h> /*drag 'n' drop*/

/*required by dynamic menus...*/
#include "LArray.h"
#include "LMenu.h"
#include "LMenu_win32.h"

/*implements mutual reference between HMENU and LMenu objects*/
typedef struct tag_LApplication_TMenuRef {
    HMENU hMenuHandle;
    void* mMenu; /* (LMenu*) */
} LApplication_TMenuRef;

typedef struct tag_LApplication_TGlobal {

	/*CheckBlocksCount() enable flag...*/
	Bool checkblockscount;

	/*handles to frame and mdi client windows*/
    HWND hwndFrame;
    HWND hwndClient;

	/*pointer to current ltoolbar*/
	void* Toolbar;

	/*handler to app icon*/
	HICON appicon;

	/*pointer to timer event handler*/
    void* TimerHandler;
    
	/*drag & drop handler and info*/
    void* DropHandler;
    i1 DroppedFile[LApplication_MAX_PATH];
    POINT DroppedPt;

	/*LDraw's global buffer*/
    HBITMAP gHBitmap;

	/*application init - required by LApplication*/
    HINSTANCE inInstance;
    i1* inArgs;

	/*required by LWindow*/
    CLIENTCREATESTRUCT clientcreate;

	/*menu accelerators*/
    HACCEL haccel;
    ui4 MenuShortcutIndex;
    ACCEL accel[LApplication_MAX_SHORTCUTS];

	/*menu bar and item handlers and params*/
    HMENU MenuBar;
    void* MenuItemHandler[LApplication_MAX_MENU_ITEMS];
	void* MenuItemParam[LApplication_MAX_MENU_ITEMS];

	/*menu mutual reference pointers*/
	LArray* MenuRef;

	/*color picker structures*/
    CHOOSECOLOR cc;
    COLORREF crCustColors[16];

} _LApplication_TGlobal;

extern _LApplication_TGlobal _LApplication_gGlobal;

#endif
#endif

/* Copyright (C) 2002 Andrea Ribichini, Camil Demetrescu

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
