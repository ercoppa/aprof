/* ============================================================================
 *  LStaticText_qt.h
 * ============================================================================

 *  Author:         (c) Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jan 30, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:34 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LStaticText_qt__
#define __LStaticText_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

/*#include "LType.h"*/
#include "LWindow.h"
#include "LWindow_qt.h"

#include <qlabel.h>

void _WrapStaticText(QLabel *inStatusBar, i1 *inString, i1 *outString, i4 inDX);

class QLeoStaticText : public QLabel
{

	public:
		QLeoStaticText(QWidget* inWidget, i1* inString);
		~QLeoStaticText();

	public:
		void *cLStaticText;


	protected :
		virtual void focusInEvent( QFocusEvent * );



};

struct tagLStaticText {
	QLeoStaticText *   mHwnd;
	void (*mHandler) ();
	ui2 mID;
	LWindow* mParentWindow;
};



#endif	/* __LSL_OS_GUI__ == __LSL_QT__ */
#endif	/* __LStaticText_qt__ */


/* Copyright (C) 2003 Gianni Bevacqua

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

