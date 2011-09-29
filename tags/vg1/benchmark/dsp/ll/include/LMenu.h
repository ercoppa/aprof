/* ============================================================================
 *  LMenu.h
 * ============================================================================

 *  Author:         (c) 2001-2003 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 20, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:17 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LMenu__
#define __LMenu__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LString.h"
#include "LException.h"

/* defines... */
#define LMenu_ID        0x8011
#define LMenu_CHECKED   0
#define LMenu_UNCHECKED 1

/* exception codes */
enum { 
    LMenu_CANT_CREATE = LMenu_ID<<16
};

/*typedefs...*/
typedef struct tagLMenu LMenu;
typedef void (*LMenu_THandler) (void* inParam);

/* functions belonging to LMenu... */
LMenu* LMenu_Create                (i1* inMenuName);
void   LMenu_Destroy               (LMenu* inMenu);

void   LMenu_AppendMenuItem        (LMenu* inMenu, i1* inItemName, ui4 inItemID, i1* inShortcut, 
                                    LMenu_THandler inItemHandler, void* inParam);
void   LMenu_InsertMenuItem        (LMenu* inMenu, ui4 inPos, i1* inItemName, ui4 inItemID, 
                                    i1* inShortcut, LMenu_THandler inItemHandler, void* inParam);
void   LMenu_AppendMenuSeparator   (LMenu* inMenu);
void   LMenu_InsertMenuSeparator   (LMenu* inMenu, ui4 inPos);
void   LMenu_AppendSubMenu         (LMenu* inMenu, LMenu* inSubMenu);
void   LMenu_InsertSubMenu         (LMenu* inMenu, LMenu* inSubMenu, ui4 inPos);
void   LMenu_RemoveMenuItemByID    (LMenu* inMenu, ui4 inItemID);
void   LMenu_RemoveMenuItemByPos   (LMenu* inMenu, ui4 inPos);
void   LMenu_EnableMenuItem        (LMenu* inMenu, ui4 inItemID);
void   LMenu_DisableMenuItem       (LMenu* inMenu, ui4 inItemID);
void   LMenu_CheckMenuItem         (LMenu* inMenu, ui4 inItemID, ui2 inCheckedUnchecked);

void   LMenu_AppendMenuToMenuBar   (LMenu* inMenu);
void   LMenu_InsertMenuInMenuBar   (LMenu* inMenu, ui4 inPos);
void   LMenu_RemoveMenuFromMenuBar (ui4 inPos);
void   LMenu_ShowMenuBar           ();
void   LMenu_HideMenuBar           ();
void   LMenu_RedrawMenuBar         ();

void   LMenu_InstallHandler        (ui4 inItemID, LMenu_THandler inItemHandler, void* inParam);
void   LMenu_InstallInitHandler    (LMenu* inMenu, LMenu_THandler inInitHandler, void* inParam);

void   LMenu_DisplayContextualMenu (LMenu* inMenu, ui4 inX, ui4 inY);

#endif

/* Copyright (C) 2001-2003 Andrea Ribichini

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
