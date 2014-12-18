#!/bin/bash

DIR=`pwd`
echo $DIR
#make clean
./autogen.sh

# default
./configure --prefix=$DIR/inst && make && make install && \

# DEBUG
#./configure --prefix=$DIR/inst  --enable-inner && make && make install && \
#valgrind --sim-hints=enable-outer --trace-children=yes  --smc-check=all-non-file  --run-libc-freeres=no --tool=memcheck --leak-check=full ./valgrind/inst/bin/valgrind --vgdb-prefix=./valgrind/ --tool=aprof ls

cd $DIR/aprof/extra/ && \
make && make install || exit -1

echo ""
echo "################################################################"
echo "#"
echo "#  You can run aprof on your program in the following way:"
echo -e "#  \t$DIR/inst/bin/valgrind --tool=aprof %YOUR_BINARY%"
echo "#  "
echo "#  You can run aprof-helper with:"
echo -e "#  \t$DIR/inst/bin/aprof-helper [OPTIONS] [report.aprof]"
echo "#"
echo "#"
echo "#  More info at: https://code.google.com/p/aprof/"
echo "#"
echo "################################################################"
echo ""

exit 0
