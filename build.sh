#!/bin/bash

directories=("./build" "./resource/download" "./resource/generate" "./resource/temp")  # Add your directories here

for directory in "${directories[@]}"
do
    if [ ! -d "$directory" ]; then
        mkdir "$directory"
    fi
done

cd ./build
cmake -DCMAKE_BUILD_TYPE=Release .. -DMODE="$1"
make -j$(nproc)