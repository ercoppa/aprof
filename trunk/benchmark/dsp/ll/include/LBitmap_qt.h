/* ============================================================================
 *  LBitmap_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jul 01, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:57 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LBitmap_qt__
#define __LBitmap_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qpixmap.h>


typedef struct tagLBitmap {
    
        QPixmap *mHbitmap;
        i4 mDX;
        i4 mDY;
        
} LBitmap;


#endif /* __LSL_OS_GUI__ == __LSL_QT__ */
#endif /* __LBitmap_qt__ */



/* Copyright (C) 2002 Gianni Bevacqua

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
