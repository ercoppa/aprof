/*
 * keys configuration
 * 
 * Last changed: $Date: 2012-08-01 12:58:45 +0200 (Wed, 01 Aug 2012) $
 * Revision:     $Rev$
 */

/*
   This file is part of aprof, an input sensitive profiler.

   Copyright (C) 2011-2012, Emilio Coppa (ercoppa@gmail.com),
                            Camil Demetrescu,
                            Irene Finocchi

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#ifndef CONF_KEYS_H
#define CONF_KEYS_H

/*
 * Three possible key types:
 * 1) read memory size (computed by aprof)
 * 2) custom value (64 bit integer)
 * 3) calling context (computed by aprof)
 */

#define FIRST_RMS      1 /* 000 000 001 */
#define FIRST_CUSTOM   2 /* 000 000 010 */
#define FIRST_CC       4 /* 000 000 100 */

#define SECOND_RMS     8 /* 000 001 000 */
#define SECOND_CUSTOM 16 /* 000 010 000 */
#define SECOND_CC     32 /* 000 100 000 */

#define THIRD_RMS     64 /* 001 000 000 */
#define THIRD_CUSTOM 128 /* 010 000 000 */
#define THIRD_CC     256 /* 100 000 000 */

/* Configuration values: combine previous cases... */
#define CUSTOM        (FIRST_CUSTOM)
#define RMS_CUSTOM    (FIRST_RMS || SECOND_CUSTOM)
#define CUSTOM_RMS    (FIRST_CUSTOM || SECOND_RMS)
#define CUSTOM_CUSTOM (FIRST_CUSTOM || SECOND_RMS)
#define RMS_CC        (FIRST_RMS || SECOND_CC)
#define RMS_CUSTOM_CC (FIRST_RMS || SECOND_CUSTOM || THIRD_CC)
#define RMS_CUSTOM_CUSTOM (FIRST_RMS || SECOND_CUSTOM || THIRD_CUSTOM)
/* add others in future... */

#endif
