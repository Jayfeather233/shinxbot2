compile_cmake() {
    local directory="$1"
    echo "Compiling CMake project in: $directory"
    cd "$directory"
    mkdir -p build  # Create build directory if not exists
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)  # Adjust the number of processes as needed
    cd ../..
}

base_directory="."

# Loop through immediate subdirectories of base_directory
for dir in "$base_directory"/*/; do
    if [ -f "$dir/CMakeLists.txt" ]; then
        compile_cmake "$dir"
    else
        echo "No CMakeLists.txt found in: $dir"
    fi
done