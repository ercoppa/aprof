/* ============================================================================
 *  LGlobals_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Apr 16, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:10 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LGlobals_qt__
#define __LGlobals_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qapplication.h>
#include <qworkspace.h>
#include <qvbox.h>
#include <qmainwindow.h>
#include <qptrlist.h>

#include "LApplication_qt.h" 

typedef struct tag_LApplication_TGlobal {



	QApplication	*mApplication;
	QLeoMainWindow  *mMainWindow;


	QWorkspace *mWorkspace;
	QVBox *mVb;

	QPtrList<QWindowsVisualizationSequence> mVisualizationSequence;
	QPtrList<QWindowsShowSequence> mShowSequence;

	void (*mDropHandler)(/*i1* inBuffer, ui4 inX, ui4 inY*/);

	void *mWidgetFocused;
	i1* inArgs;

	ui2 mWidgetFocusedId;
	Bool mBringToFront;
	Bool mCloseApplication;
	Bool mEndApplication;

} _LApplication_TGlobal;

extern _LApplication_TGlobal _LApplication_gGlobal;


#endif /* __LSL_OS_GUI__ == __LSL_QT__ */
#endif /* __LGlobals_qt__ */



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

