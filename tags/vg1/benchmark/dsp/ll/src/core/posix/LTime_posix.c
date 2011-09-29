/* ===========================================================================
 *  LTime_posix.c
 * ===========================================================================

 *  Author:         (c) 2001-2003 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 29, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:05:59 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "LConfig.h"

#if __LSL_OS_CORE__ == __LSL_POSIX__

#include "LTime.h"

#if __LSL_PLATFORM__ == __LSL_MacOSX_PowerPC__ || __LSL_PLATFORM__ == __LSL_Linux_x86__
#include <sys/param.h>
#endif

/* ============================================================================
 *  MACOS9
 * ============================================================================
*/
#if __LSL_PLATFORM__ == __LSL_MacOS9_PowerPC__

#include <time.h>

/* ----------------------------------------------------------------------------
 *  GetUserTime
 * ----------------------------------------------------------------------------
*/
f8 LTime_GetUserTime(){
    return (f8)clock()/CLOCKS_PER_SEC;
}

/* ============================================================================
 *  BSD
 * ============================================================================
*/
#elif defined(BSD)

#include <sys/time.h>
#include <sys/resource.h>

/* ----------------------------------------------------------------------------
 *  GetUserTime
 * ----------------------------------------------------------------------------
*/
f8 LTime_GetUserTime() {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (f8)ru.ru_utime.tv_sec + (f8)ru.ru_utime.tv_usec*1e-6;
}

/* ============================================================================
 *  OTHERS
 * ============================================================================
*/
#else

#if __LSL_PLATFORM__ != __LSL_Win32_x86__

#include <sys/types.h>
#include <sys/times.h>

/* ----------------------------------------------------------------------------
 *  GetUserTime
 * ----------------------------------------------------------------------------
*/
f8 LTime_GetUserTime() {
    struct tms rusage;
    times(&rusage);
    return (f8)rusage.tms_utime/(f8)HZ;
}

#endif

#endif

#endif

/* Copyright (C) 2001-2003 Camil Demetrescu

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
