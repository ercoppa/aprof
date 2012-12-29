/*
 * Client request handler
 * 
 * Last changed: $Date: 2012-08-01 02:00:39 +0200 (Wed, 01 Aug 2012) $
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

#include "aprof.h"

Bool APROF_(client_req)(ThreadId tid, UWord * arg, UWord * ret) {

	/* Is this client request for aprof? */
	if (!VG_IS_TOOL_USERREQ('V','A',arg[0])) return False;
	
	return True;

}
