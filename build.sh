#!/bin/sh

if [ ! -x "./build"]; then
mkdir "./build"
fi

cd ./build
make
cd ..
./build/cq_bot