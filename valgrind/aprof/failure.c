/*
 * Failure/error handler 
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#include "aprof.h"

void failure(char * msg) {

	VG_(printf)("%s\n", msg);
	VG_(exit)(1);

}
