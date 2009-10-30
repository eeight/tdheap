#!/bin/sh
./autogen.sh && ./configure --prefix=$(pwd)/inst --enable-only64bit CC=gcc-4.3.2 CXX=g++-4.3.2
