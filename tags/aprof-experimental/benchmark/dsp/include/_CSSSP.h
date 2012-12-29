/* ============================================================================
 *  _CSSSP.h
 * ============================================================================

 *  Author:         (c) 2003 Camil Demetrescu, Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        January 9, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:53 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "CSSSP.h"

void _CSSSP_UI4(LGraph* inGraph, LGraph_TNode* inSource, LEdgeInfo* inWeightArray,
                LNodeInfo** outDistArray, LHeap* inPQ, LNodeInfo* inPQIdx);

/* Copyright (C) 2003 Camil Demetrescu, Stefano Emiliozzi

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
