#!/bin/bash

DIR=`pwd`
echo $DIR
#make clean
./autogen.sh
./configure --prefix=$DIR/inst && make && make install && \
cd $DIR/aprof/extra/ && \
make && cp aprof-helper ../../inst/bin/ || exit -1

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
