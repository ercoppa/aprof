/* ============================================================================
 *  LDiskStack.h
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu, Francesco Mungiguerra
 *  License:        See the end of this file for license information
 *  Created:        March 27, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:05 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDiskStack__
#define __LDiskStack__

#include "LType.h"

/* COMPONENT ID */
#define LDiskStack_ID 0x8016

/* TYPEDEFS */
typedef struct LDiskStack LDiskStack;
    
/* PUBLIC FUNCTIONS */
LDiskStack*     LDiskStack_New              (const ui4 inBufferSize);
void            LDiskStack_Delete           (LDiskStack** ThisA);

void            LDiskStack_Push             (LDiskStack* This, const void* inBlock, const ui4 inSize);
void            LDiskStack_Pop              (LDiskStack* This, void* outBlock, const ui4 inSize);
Bool            LDiskStack_IsEmpty          (LDiskStack* This);
ui4             LDiskStack_GetSize          (LDiskStack* This);

#endif


/* Copyright (C) 2001 Camil Demetrescu, Francesco Mungiguerra

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


