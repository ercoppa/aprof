/* ============================================================================
 *  LChar.h
 * ============================================================================

 *  Author:         (c) 2001 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 28, 2001
 *  Module:         LL

 *  Last changed:   $Date: 2010/04/23 16:04:58 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __LChar__
#define __LChar__

#include "LConfig.h"
#include "LType.h"
#include <ctype.h>

/* COMPONENT ID */
#define LChar_ID    0x800B

#define LChar_IsAlnum(c)    ((Bool)isalnum(c))
#define LChar_IsAlpha(c)    ((Bool)isalpha(c))
#define LChar_IsCntrl(c)    ((Bool)iscntrl(c))
#define LChar_IsDigit(c)    ((Bool)isdigit(c))
#define LChar_IsGraph(c)    ((Bool)isgraph(c))
#define LChar_IsLower(c)    ((Bool)islower(c))
#define LChar_IsPrint(c)    ((Bool)isprint(c))
#define LChar_IsPunct(c)    ((Bool)ispunct(c))
#define LChar_IsSpace(c)    ((Bool)isspace(c))
#define LChar_IsUpper(c)    ((Bool)isupper(c))
#define LChar_IsXDigit(c)   ((Bool)isxdigit(c))
#define LChar_ToLower(c)    ((i2)tolower(c))
#define LChar_ToUpper(c)    ((i2)toupper(c))

#if 0
#define LChar_IsAlpha(c)    (    ((c)>=(char)'a' && (c)<=(char)'z') || ( (c)>=(char)'A' && (c)<=(char)'Z') )
#define LChar_IsDigit(c)    (  ((c)>=(char)'0' && (c)<=(char)'9')  /* (Bool)isdigit(c)*/)
#define LChar_IsSpace(c)    ( ( (c)==(char) ' ' || (c)==10)) /*(Bool)isspace(c)*/
#endif

#endif


/* Copyright (C) 2001 Camil Demetrescu

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
