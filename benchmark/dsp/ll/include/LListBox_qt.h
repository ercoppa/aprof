/* ============================================================================
 *  LListBox_qt.h
 * ============================================================================

 *  Author:         (c) Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Aug 13, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:15 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LListBox_qt__
#define __LListBox_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qlistbox.h>

#include "LListBox.h"
#include "LType.h"
#include "LWindow.h"
#include "LWindow_qt.h"



class QLeoListBox : public  QListBox
{
	Q_OBJECT

	public:
		QLeoListBox(QWidget* inWidget, char *inString);
		~QLeoListBox();


	public slots:
		void slotHighlighted(int inIndex);


	public:
		ui1 cCurrentItemStatus;
		void *cLListBox;

	
	protected :
		virtual void focusInEvent( QFocusEvent * );
		virtual void mouseDoubleClickEvent( QMouseEvent * );



};


struct tagLListBox {

	QLeoListBox * mHwnd;
	void (*mHandler) (LListBox* );
	ui2 mID;
	LWindow* mParentWindow;

};

#endif	/* __LSL_OS_GUI__ == __LSL_QT__ */
#endif	/* __LListBox_qt__ */


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

