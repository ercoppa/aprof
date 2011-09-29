/* ============================================================================
 *  LDraw_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jun 27, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:06 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LDraw_qt__
#define __LDraw_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qframe.h>
#include <qpen.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qmainwindow.h>
#include <qimage.h>

#include "LWindow.h"
#include "LWindow_qt.h"
#include "LDraw.h"

#define FRAME_X		2
#define FRAME_Y		2

class QLeoDraw : public QFrame
{

	public:

	QLeoDraw(QChildWindow *);
	~QLeoDraw();

	protected  :

	virtual void paintEvent(QPaintEvent *);
	virtual void mousePressEvent(QMouseEvent *);
	virtual void mouseMoveEvent(QMouseEvent *);


	public :
	void *cLDraw;
	
	protected :
	virtual void focusInEvent( QFocusEvent * );
};

struct tagLDraw {
        QLeoDraw* mQLeoDraw;
	ui2 mID;
	LDraw_TBufferType mType;
        QPainter *mQPainter;
        QPixmap *mQPixmap;
        QPen * mQPen;
        QBrush * mQBrush;
        int mmaxX;
        int mmaxY;
        int mX;
        int mY;
	Bool mFrame;
	Bool (*mMouseDownHandler)(/*struct tagLDraw*, int, int*/);
	Bool (*mRefreshHandler)(/*struct tagLDraw**/);
	Bool (*mMouseMoveHandler) (/*struct tagLWindow*, int, int*/);
	LWindow* mParentWindow;

	LCursor_TShape mCursorShape;

};

typedef Bool (*_LDraw_THandler) ();

//*typedef bool (*_LDraw_THandler_mouse) (/*LDraw*, int, int*/);
//typedef bool (*_LDraw_THandler_refresh) (/*LDraw**/);
//typedef bool (*_LDraw_THandler_mouse_move) (/*LDraw*, /nt, int*/);



#endif /* __LSL_QT__ */
#endif /* __LDraw_qt__ */


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


