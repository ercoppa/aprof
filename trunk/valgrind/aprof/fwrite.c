/*
 * Aprof implementation of libc fopen, fwrite, fclose, fflush functions 
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

/* Note: fwrite() is not provided by valgrind, so... */

FILE * ap_fopen(char * name){
	
	/*
	char name[150];
	int pid = VG_(getpid)();
	VG_(sprintf)(name, "aprof_log_%d-%u.log", pid, tid);
	*/
	
	SysRes res = VG_(open)(name, VKI_O_CREAT|VKI_O_WRONLY|VKI_O_TRUNC, VKI_S_IRUSR|VKI_S_IWUSR);
	int file = (Int) sr_Res(res);
	AP_ASSERT(file != -1, "Can't create a log file.")
	
	FILE * f = VG_(malloc)("log_file", sizeof(FILE));
	f->file = file;
	f->fw_pos = 0;
	
	return f;

}

void ap_fflush(FILE * f) {
	
	unsigned int bw = 0, bf = 0, size = f->fw_pos;
	do {
		bf = VG_(write)(f->file, f->fw_buffer + bw, size - bw);
		AP_ASSERT(bf != -1, "Error during writing\n");
		bw += bf;
	} while(bw < size);
	
	f->fw_pos = 0;

}

void ap_fwrite(FILE * f, char * buffer, unsigned int size) {

	if (buffer == NULL || size == 0) return;
	/* We suppose size always less than BUFFER_SIZE */
	
	if (BUFFER_SIZE - f->fw_pos <= size) ap_fflush(f);
	VG_(strncpy)(f->fw_buffer + f->fw_pos, buffer, size);
	f->fw_pos += size;

}

void ap_fclose(FILE * f) {
	ap_fflush(f);
	VG_(close)(f->file);
	VG_(free)(f);
}
