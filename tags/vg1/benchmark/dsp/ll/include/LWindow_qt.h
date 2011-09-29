/* ============================================================================
 *  LWindow_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Apr 16, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:42 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LWindow_qt__
#define __LWindow_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qmainwindow.h>
#include <qworkspace.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qframe.h>
#include <qpen.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qevent.h>
#include <qtextedit.h>
#include <qscrollbar.h>
#include <qptrlist.h>

#include"LType.h"
#include"LWindow.h"



typedef struct tagLWindow LWindow;

typedef Bool (*_LWindow_THandler) ();

typedef struct tagWidgetStruct {

	QWidget *mWidgetPtr;
	void (*mClickHandle)();
	struct tagWidgetStruct* mNext;

} *LWidgetStruct;

class QWidgetDestroyList {

	public:
		QWidgetDestroyList();
		~QWidgetDestroyList();
	
	public :
		void *mWidgetListPtr;
		ui2 mID;
 
} ;


// CLASS DEFINITIONS

class QChildWindow : public  QMainWindow
{ 
	public:
		QChildWindow(WFlags);
		~QChildWindow();
     
	protected  :
		virtual void closeEvent( QCloseEvent *  );
		virtual void resizeEvent ( QResizeEvent * );
		virtual void focusInEvent( QFocusEvent * );
		virtual void mousePressEvent( QMouseEvent * );
		virtual void mouseMoveEvent( QMouseEvent * );

	public :
		void *cLWindow;
		QPtrList<QWidgetDestroyList> cWidgetList;

};

struct tagLWindow {

    QChildWindow* mHwnd;
    LWindow_TWindowType mType;
    Bool (*mCloseHandler)();
    Bool (*mMouseDownHandler)();
    Bool (*mMouseMoveHandler) ();
    Bool (*mActivateHandler)();
    Bool (*mResizeHandler)();
    Bool (*mSetFocusHandler) ();
    Bool mStatusBar;
    Bool Resizable;
    i4   mWidth;
    i4   mLength;
    ui2	 mID;
	LCursor_TShape mCursorShape;
    void* mUserData; 

};

void _LWindow_DestroyWidget(void *, ui2);

#endif /* __LSL_QT__ */
#endif /* __LWindow_qt__ */


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


