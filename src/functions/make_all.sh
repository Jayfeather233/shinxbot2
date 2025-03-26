#!/bin/bash

compile_cmake() {
    local directory="$1"
    echo "Compiling CMake project in: $directory"
    cd "$directory"
    mkdir -p build  # Create build directory if not exists
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j1
    cd ../..
}

export -f compile_cmake  # Export function for parallel execution

base_directory="."
nproc=$(nproc)

# Find directories containing CMakeLists.txt and run compile_cmake in parallel
find "$base_directory" -mindepth 1 -maxdepth 1 -type d -exec test -f "{}/CMakeLists.txt" \; -print | \
    xargs -I{} -P "$nproc" bash -c 'compile_cmake "$@"' _ {}
