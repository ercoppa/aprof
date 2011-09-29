/* ============================================================================
 *  _CDSSSP.h
 * ============================================================================

 *  Author:         (C) 2003 Stefano Emiliozzi
 *  License:        See the end of this file for license information
 *  Created:        March 20, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:52 $
 *  Changed by:     $Author: demetres $   
 *  Revision:       $Revision: 1.1 $    
*/

#ifndef ___CDSSSP__
#define ___CDSSSP__

#include "CDSSSP.h"
void CDSSSP_IncreaseEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inDelta);
void CDSSSP_DecreaseEdge(CDSSSP* This, LGraph_TEdge* inEdgeUV, ui4 inDelta);
#endif

/* Copyright (C) 2003 Stefano Emiliozzi 

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

