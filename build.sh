#!/bin/sh

mkdir ./build
mkdir ./resource/download
mkdir ./resource/generate
mkdir ./resource/temp

cd ./build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)