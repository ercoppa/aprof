#!/bin/bash

#make clean
./autogen.sh
./configure --prefix=`pwd`/inst
make
make install

exit 0
