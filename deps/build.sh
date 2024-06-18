#!/usr/bin/env bash

cd `dirname $0`
cd ./sqlite-autoconf-3460000
mkdir build
cd build
../configure && make
