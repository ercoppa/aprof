/* ============================================================================
 *  LThread.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Nov 27, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:38 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LThread__
#define __LThread__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"

/* typedefs... */
typedef struct tagLThread LThread;
typedef void (*LThread_THandler) (void* inParam);

/* defines... */
#define LThread_ID 0x802C

/* exception codes */
enum { 
    LThread_CANT_CREATE = LThread_ID<<16
};

/* functions belonging to LThread */
LThread* LThread_New     (LThread_THandler inHandler, void* inParam);
void     LThread_Suspend (LThread* inT);
void     LThread_Resume  (LThread* inT);
void     LThread_Delete  (LThread* inT);
void     LThread_Wait    (LThread* inT);

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

