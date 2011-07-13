/*
 * Failure/error handler 
 * 
 * Last changed: $Date: 2011-06-18 21:08:13 +0200 (Sat, 18 Jun 2011) $
 * Revision:     $Rev: 61 $
 */

#include "aprof.h"

void failure(char * msg) {

	VG_(printf)("%s\n", msg);
	VG_(exit)(1);

}
