/* ============================================================================
 *  LStatusBar_qt.h
 * ============================================================================

 *  Author:         (c) Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Dec 24, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:34 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LStatusBar_qt__
#define __LStatusBar_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include "LStatusBar.h"
#include "LWindow_qt.h"

#include <qstatusbar.h>

#define STATUS_BAR_HEIGHT	25
#define MAX_PART_NUMBER		20

class QLeoStatusBar : public  QStatusBar
{

	public:
		QLeoStatusBar(QWidget* inWidget, i1* inString);
		~QLeoStatusBar();

	public:
		void *cLStatusBar;


	protected :
		virtual void focusInEvent( QFocusEvent * );



};


struct tagLStatusBar {

	 QLeoStatusBar 	*mStatusBar;
	 ui2 		mID;
	 LWindow	*mParentWindow;

};


#endif /* __LSL_OS_GUI__ == __LSL_QT__ */
#endif /* __LStatusBar_qt__ */


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
