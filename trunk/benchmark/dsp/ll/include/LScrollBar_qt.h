/* ============================================================================
 *  LScrollBar_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Aug 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:29 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LScrollBar_qt__
#define __LScrollBar_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include "LScrollBar.h"
#include "LWindow_qt.h"
#include <qscrollbar.h>

class QLeoScrollBar : public  QScrollBar
{
	Q_OBJECT

	public:
	QLeoScrollBar(Orientation inOrientation, QMainWindow *inWindow);
	~QLeoScrollBar();

	public slots:
	void slotLineUp();
	void slotLineDown();
	void slotPageUp();
	void slotPageDown();
	void slotThumbPosition(int inPos);
	void slotThumbTracking(int inPos);
	
	public :
	void *cLScrollBar;
	
	protected :
	virtual void focusInEvent( QFocusEvent * );

};


struct tagLScrollBar {
    QLeoScrollBar *mScrollBar;
    void (*mHandler) (LScrollBar* , LScrollBar_TEvent , ui2 );
    ui2 mID;
    LWindow* mParentWindow;
};

typedef void (*_LScrollBar_THandler) (LScrollBar* , LScrollBar_TEvent , ui2);

#endif /* __LSL_OS_GUI__ == __LSL_QT__ */
#endif /* __LScrollBar_qt__ */



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



