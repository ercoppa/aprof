/* ============================================================================
 *  LDialog.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Feb 16, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:04 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDialog__
#define __LDialog__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"
#include "LWindow.h"
#include "LFile.h"

/* typedefs... */
typedef enum tagLDialog_TMBReturnID {
    LDialog_MBIDABORT,
    LDialog_MBIDCANCEL,
    LDialog_MBIDIGNORE,
    LDialog_MBIDNO,
    LDialog_MBIDOK,
    LDialog_MBIDRETRY,
    LDialog_MBIDYES
} LDialog_TMBReturnID;

typedef enum tagLDialog_TMBButtons {
    LDialog_MBABORTRETRYIGNORE,
    LDialog_MBOK,
    LDialog_MBOKCANCEL,
    LDialog_MBRETRYCANCEL,
    LDialog_MBYESNO,
    LDialog_MBYESNOCANCEL
} LDialog_TMBButtons;

typedef enum tagLDialog_TMBIcon {
    LDialog_MBICONEXCLAMATION,
    LDialog_MBICONINFORMATION,
    LDialog_MBICONQUESTION,
    LDialog_MBICONSTOP
} LDialog_TMBIcon;

/* defines... */
#define LDialog_MAX_TITLE 512
#define LDialog_ID 0x8014 

/* exception codes */
enum { 
    LDialog_CANT_CREATE = LDialog_ID<<16
};

/* functions belonging to LDialog */
LDialog_TMBReturnID LDialog_DisplayMessageBox (LWindow* inOwner, i1* inText, i1* inTitle,
                                               LDialog_TMBButtons inButtons, LDialog_TMBIcon inIcon);
Bool                LDialog_GetSaveFileName   (LWindow* inOwner, i1* inFilters[], ui1 inNumFilters, 
                                               i1 inBuffer[LFile_MAX_PATHNAME_LEN]); 
Bool                LDialog_GetOpenFileName   (LWindow* inOwner, i1* inFilters[], 
                                               ui1 inNumFilters, i1 inBuffer[LFile_MAX_PATHNAME_LEN]);
ui4                 LDialog_ColorPicker       (LWindow* inOwner);

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

