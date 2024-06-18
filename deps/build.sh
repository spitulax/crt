#!/usr/bin/env bash

cd `dirname $0`
cd ./sqlite3
mkdir build
cd build
../configure && make -j
