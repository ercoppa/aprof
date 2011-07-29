/*
 * Make code written for glibc compatible with valgrind
 * 
 * Last changed: $Date$
 * Revision:     $Rev$
 */

#ifndef _GLIBC_VALGRIND_H
#define _GLIBC_VALGRIND_H

#ifndef GLIBC

#define exit         VG_(exit)
#define printf       VG_(printf)
#define calloc(a, b) VG_(calloc)("aprof", a, b)
#define malloc(a)    VG_(malloc)("aprof", a)
#define free         VG_(free)
#define qsort        VG_(ssort)

#endif

#endif
