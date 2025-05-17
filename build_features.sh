#!/bin/bash

cd ./src/functions
python3 generate_cmake.py
./make_all.sh
cd ../..

cd ./src/events
python3 generate_cmake.py
./make_all.sh
cd ../..