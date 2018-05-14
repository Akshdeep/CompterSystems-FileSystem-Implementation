#!/bin/sh

tar -xf fuse-2.9.7.tar.gz
cd fuse-2.9.7
mkdir install
./configure --prefix=$(pwd)/install
make -j 4
make install -k
