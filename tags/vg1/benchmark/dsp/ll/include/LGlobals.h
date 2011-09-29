/* ============================================================================
 *  LGlobals.h
 * ============================================================================

 *  Author:         (c) 2001 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 20, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:09 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LGlobals__
#define __LGlobals__

#include "LType.h"
#include "LConfig.h"

/*GLOBAL DEFINES...*/
#define LGlobals_ID 0x8012

/*max number of menu items*/
#define LApplication_MAX_MENU_ITEMS 1000

/*max number of menu shortcuts*/
#define LApplication_MAX_SHORTCUTS 1000

/*max number of menus*/
#define LApplication_MAX_MENUS 100

/*max length of filename*/
#define LApplication_MAX_PATH 255

#if __LSL_OS_GUI__ == __LSL_Win32__
#include "LGlobals_win32.h"
#elif __LSL_OS_GUI__ == __LSL_QT__
#include "LGlobals_qt.h"
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
