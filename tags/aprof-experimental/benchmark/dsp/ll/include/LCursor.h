/* ============================================================================
 *  LCursor.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Jul 24, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:01 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LCursor__
#define __LCursor__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"

/* defines... */
#define LCursor_ID 0x801D

/* typedefs... */

typedef enum tagLCursor_TShape {
    LCursor_APPSTARTING,
    LCursor_ARROW,
    LCursor_CROSS,
    LCursor_IBEAM,
    LCursor_NO,
    LCursor_SIZEALL,
    LCursor_SIZENESW,
    LCursor_SIZENS,
    LCursor_SIZENWSE,
    LCursor_SIZEWE,
    LCursor_UPARROW,
    LCursor_WAIT
} LCursor_TShape;

/* functions belonging to LCursor */
void LCursor_GetPosition (ui4 *outX, ui4 *outY);

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

