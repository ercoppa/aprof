#!/bin/bash

#make clean
./autogen.sh
./configure --prefix=`pwd`/inst --enable-inner
make
make install

# run like
#  valgrind --sim-hints=enable-outer --trace-children=yes  --smc-check=all-non-file  --run-libc-freeres=no --tool=memcheck --leak-check=full ../development/valgrind/inst/bin/valgrind --vgdb-prefix=../development/valgrind --tool=aprof ls

exit 0
