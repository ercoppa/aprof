/* ============================================================================
 *  LTime.h
 * ============================================================================

 *  Author:         (c) 2001,2002 Camil Demetrescu, Stefano Emiliozzi, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        Dec 19, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:39 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LTime__
#define __LTime__

#include "LType.h"

/* COMPONENT ID */
#define LTime_ID  0x8013

/* TYPEDEFS */
typedef struct tagLTime { /* 10 bytes */
	ui2 mYear;
	ui1 mMonth;
	ui1 mDay;
	ui1 mHour;
	ui1 mMin;
	ui1 mSec;
	ui2 mMsec;
} LTime;

/* PUBLIC FUNCTIONS */
f8      LTime_GetUserTime     ();
LTime   LTime_GetCalendarTime ();
i1      LTime_Compare         (LTime t1, LTime t2);

#define LTime_GetYear(t)  ((t).mYear)
#define LTime_GetMonth(t) ((t).mMonth)
#define LTime_GetDay(t)   ((t).mDay)
#define LTime_GetHour(t)  ((t).mHour)
#define LTime_GetMin(t)   ((t).mMin)
#define LTime_GetSec(t)   ((t).mSec)
#define LTime_GetMsec(t)  ((t).mMsec)

#endif


/* Copyright (C) 2001-2003 Camil Demetrescu, Stefano Emiliozzi, Andrea Ribichini

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
