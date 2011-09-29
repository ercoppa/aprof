/* ============================================================================
 *  LRadioButton_qt.h
 * ============================================================================

 *  Author:         (c) 2002 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jun 26, 2002
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:25 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#ifndef __LRadioButton_qt__
#define __LRadioButton_qt__

#if __LSL_OS_GUI__ == __LSL_QT__

#include <qradiobutton.h>

#include "LRadioButton.h"
#include "LWindow.h"
#include "LWindow_qt.h"

class QLeoRadioButton : public  QRadioButton
{
	Q_OBJECT

	public:

	QLeoRadioButton(char *inString, QWidget* inWidget);

	~QLeoRadioButton();

	public slots:
	void slotRadioButtonClicked();
	

	public:
	void *cLRadioButton;
	
	protected :
	virtual void focusInEvent( QFocusEvent * );
	

};


typedef struct tagLRadioButton {

    QLeoRadioButton* mRadioButton;
    ui2 mID;
    void (*mHandler) ( LRadioButton * );
    LWindow* mParentWindow;

};


#endif /* __LSL_QT__ */
#endif /* __LRadioButton_qt__ */



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

