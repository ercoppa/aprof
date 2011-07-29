/* ============================================================================
 *  LApplication_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jul 02, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:54 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LApplication_qt__
#define __LApplication_qt__

#if __LSL_OS_GUI__ == __LSL_QT__


#include <qmainwindow.h>

#include <qpixmap.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qdragobject.h>

#include "LType.h"
#include "LWindow_qt.h"
#include "LWindow.h"

#include "LMenu_qt.h"
#include "LBitmap_qt.h"


class QWindowsVisualizationSequence {

	public:
		QWindowsVisualizationSequence();
		~QWindowsVisualizationSequence();

	public :
		/*Bool mShow;
		Bool mBringToFront;*/
		LWindow *mWindowPtr;

};

class QWindowsShowSequence {

	public:
		QWindowsShowSequence();
		~QWindowsShowSequence();

	public :
		Bool mShow;
		Bool mBringToFront;
		LWindow *mWindowPtr;

};


typedef struct tagMenuHandlerStruct {

    int mMenuId;
    void (*mMenuHandler)(void* inParam);
	void *mMenuHandlerParam;
    struct tagMenuHandlerStruct* mNext;

} LMenuHandlerStruct;


class QMenuDestroyList {


	public:
	QMenuDestroyList();
	~QMenuDestroyList();

	public :
	LMenu *mMenuListPtr;


} ;

class QBitmapDestroyList {


	public:
		QBitmapDestroyList();
		~QBitmapDestroyList();

	public :
		LBitmap *mBitmapListPtr;


} ;



class QLeoMainWindow : public  QMainWindow
{
    Q_OBJECT

    public:

    	QLeoMainWindow();

    	~QLeoMainWindow();

    public slots:

		void slotMenuPressed( int );
		void slotTimer();

    public:

		struct tagMenuHandlerStruct *cFirstMenuHandlerStruct;
		QPixmap     *cGlobalBuffer;
		QPainter    *cGraphicsContext;
    	QTimer      *cApplicationTimer;
    	void (*cTimer)();
    	void (*cMenuInitHandler[50])();

    	QPtrList<QMenuDestroyList> cMenuList;
    	QPtrList<QBitmapDestroyList> cBitmapList;
		

    protected :
    	virtual void customEvent( QCustomEvent *inEvent );
    	virtual void closeEvent( QCloseEvent *inEvent );
		virtual void dragEnterEvent( QDragEnterEvent *inEvent );
		virtual void dragLeaveEvent( QDragLeaveEvent *inEvent );
		virtual void dragMoveEvent( QDragMoveEvent *inEvent );
		virtual void dropEvent( QDropEvent *inEvent );


};

class KillerFilterApp : public QObject
{
	protected :
		bool eventFilter( QObject *inObject, QEvent *inEvent );

};




void _LApplication_BringWindowsToFront();
void _LApplication_DispatchBringToFront();

#endif /* __LSL_OS_GUI__ == __LSL_QT__ */
#endif /* __LApplication_qt__ */



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


