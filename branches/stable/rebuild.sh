#!/bin/bash

make && make install && \
cd aprof/extra/ && \
make && make install || exit -1

exit 0
