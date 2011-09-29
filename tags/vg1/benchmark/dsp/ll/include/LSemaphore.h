/* ============================================================================
 *  LSemaphore.h
 * ============================================================================

 *  Author:         (c) 2002 Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 3, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:30 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LSemaphore__
#define __LSemaphore__

/* includes... */
#include "LConfig.h"
#include "LType.h"
#include "LGlobals.h"
#include "LMemory.h"
#include "LException.h"

/* typedefs... */
typedef struct tagLSemaphore LSemaphore;

/* defines... */
#define LSemaphore_ID 0x802D

/* exception codes */
enum { 
    LSemaphore_CANT_CREATE = LSemaphore_ID<<16
};

/*functions belonging to LSemaphore*/
LSemaphore* LSemaphore_New    (ui4 inVal);
void        LSemaphore_Signal (LSemaphore* inSem);
void        LSemaphore_Wait   (LSemaphore* inSem);
void        LSemaphore_Delete (LSemaphore* inSem);

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

