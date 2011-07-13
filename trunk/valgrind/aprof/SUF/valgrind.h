/*
 * Setup UF in order to work with Valgrind
 * 
 * Last changed: $Date: 2011-06-18 21:08:13 +0200 (Sat, 18 Jun 2011) $
 * Revision:     $Rev: 61 $
 */

#ifndef _VALGRING_H
#define  _VALGRIND_H

#ifndef GLIBC

#include "pub_tool_basics.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#define exit         VG_(exit)
#define printf       VG_(printf)
#define calloc(a, b) VG_(calloc)("aprof", a, b)
#define malloc(a)    VG_(malloc)("aprof", a)
#define free         VG_(free)
#define qsort        VG_(ssort)
#define size_t       unsigned int

#endif

#endif
