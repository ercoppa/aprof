/* =============================================================================
 *  LThread_qt.h
 * =============================================================================

 *  Author:         (c) 2003 Gianni Bevacqua
 *  License:        See the end of this file for license information
 *  Created:        Jan 06, 2003
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:39 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/



#if __LSL_OS_GUI__ == __LSL_QT__

#include <pthread.h>

#include "LThread.h"
#include "LWindow.h"
#include "LWindow_qt.h"

struct tagLThread {

	pthread_t mThreadHandle;
	
};

typedef void (*LThread_THandlerQT) (void* );


#endif	/* __LSL_OS_GUI__ == __LSL_QT__ */


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
