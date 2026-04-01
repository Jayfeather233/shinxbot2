#!/bin/bash
set -euo pipefail

directories=("./build" "./resource/download" "./resource/generate" "./resource/r_color")

for directory in "${directories[@]}"; do
    if [ ! -d "$directory" ]; then
        mkdir -p "$directory"
    fi
done

cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release -DMODE="${1:-}"
cmake --build ./build -j"$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))"