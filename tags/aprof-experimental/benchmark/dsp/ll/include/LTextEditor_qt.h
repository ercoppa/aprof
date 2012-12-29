/* ============================================================================
 *  LTextEditor_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Oct 10, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:36 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LTextEditor_qt__
#define __LTextEditor_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include "LWindow.h"
#include "LWindow_qt.h"
#include "LTextEditor.h"

#include <string.h>
#include <stdlib.h>

#include <qmainwindow.h>
#include <qtextedit.h>

class QLeoTextEdit : public  QTextEdit
{
	Q_OBJECT

	public:
	QLeoTextEdit( QMainWindow *inWindow, i1* inString );
	~QLeoTextEdit();

	public slots:
	void slotReturnPressed();

	public :

	void *cLTextEdit;
	

	protected :
	virtual void focusInEvent( QFocusEvent * );
	virtual void mouseMoveEvent(QMouseEvent * );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void keyPressEvent ( QKeyEvent *  );


};

struct tagLTextEditor {

	QLeoTextEdit *mHwndEdit;
	void (*mHandler) ();
	ui2 mID;
	LWindow* mParentWindow;

	ui4 mHCaret;
	Bool (*mLCLKHandler) ();
	Bool (*mLDBLCLKHandler) ();
	Bool (*mRCLKHandler) ();
	Bool (*mCHARHandler) ();
	Bool (*mKEYDOWNHandler) ();
	
	Bool mLCLKLater;
	Bool mLDBLCLKLater;
	Bool mRCLKLater;
	Bool mKEYDOWNLater;

};

class KillerFilter : public QObject
{
	protected :
		bool eventFilter(QObject * object, QEvent * event);

};


void  _itoa(int n, i1 *s);
void  _reverse(i1 *s);

typedef Bool (*_LTextEditor_THandler) (/*LTextEditor **/);

i4 _VirtKey2LTxtEdt (int inVirtKey);

#endif /* __LSL_QT__ */
#endif /* __LTextEditor_qt__ */



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

