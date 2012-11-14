/*
 * Aprof implementation of libc fopen, fwrite, fclose, fflush functions 
 * 
 * Last changed: $Date$
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

/* Note: fwrite() & co is not provided by valgrind, so... */

FILE * APROF_(fopen)(char * name){
	
	SysRes res = VG_(open)(name, VKI_O_EXCL|VKI_O_CREAT|VKI_O_WRONLY, VKI_S_IRUSR|VKI_S_IWUSR);
	int file = (Int) sr_Res(res);
	if (file <= 0) return NULL;
	
	//AP_ASSERT(file >= 0, "Can't create a log file.")
	
	FILE * f = VG_(malloc)("log_file", sizeof(FILE));
	f->file = file;
	f->fw_pos = 0;
	
	return f;

}

void APROF_(fflush)(FILE * f) {
	
	unsigned int bw = 0, bf = 0, size = f->fw_pos;
	do {
		bf = VG_(write)(f->file, f->fw_buffer + bw, size - bw);
		AP_ASSERT(bf != -1, "Error during writing\n");
		bw += bf;
	} while(bw < size);
	
	f->fw_pos = 0;

}
void APROF_(fwrite)(FILE * f, char * buffer, unsigned int size) {

	if (buffer == NULL || size == 0) return;
	
	/* We suppose size always less than BUFFER_SIZE */
	AP_ASSERT(size < BUFFER_SIZE, "Error during writing\n");
	
	if (BUFFER_SIZE - f->fw_pos <= size) APROF_(fflush)(f);
	VG_(strncpy)(f->fw_buffer + f->fw_pos, buffer, size);
	f->fw_pos += size;

}

void APROF_(fclose)(FILE * f) {
	
	APROF_(fflush)(f);
	VG_(close)(f->file);
	VG_(free)(f);

}
