#                           -------------------------------------
#                           Dynamic all-pairs shortest paths experimental package (dsp-1.0)
#                           Makefile (c) 2002-2003 Camil Demetrescu
#                           All rights reserved.
#                           Last changed:   $Date: 2010/11/08 11:58:21 $
#                           Changed by:     $Author: demetres $
#                           Revision:       $Revision: 1.3 $

#                           -------------------------------------

#                           -------------------------------------
#                           Macros
#                           -------------------------------------
LL_PATH =                   ../ll/
C  =                        gcc  -I$(LL_PATH)include -Iinclude -I../include -lm
LL =                        $(LL_PATH)src/core/common/*.c $(LL_PATH)src/core/posix/*.c -lm

#                           -------------------------------------
#                           Rules
#                           -------------------------------------


all:
	                        cd bin; $(C) -o dsp $(LL) ../src/*.c

instrument:
	                        cd bin; $(C) -o dsp-ins $(LL) ../src/*.c \
	                        -finstrument-functions\
	                        -DINSTRUMENT_FUNCTIONS=1

test:
	                        cd bin; $(C) -o TestLHeap $(LL) ../test/TestLHeap.c

gprof:
	                        cd bin; $(C) -pg -o dsp $(LL) ../src/*.c

clean:
	                        cd bin; rm -f *

pack:
	                        $(MAKE) clean; cd ..; tar cf dsp-1.0.tar dsp-1.0; gzip dsp-1.0.tar

valgrind:
	cd bin; $(C) -o dsp $(LL) ../src/*.c -finstrument-functions ../../../gcc-instrument/instr-gcc.c
