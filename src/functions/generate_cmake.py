pattern = """
cmake_minimum_required(VERSION 3.10)

project({CNAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../../../lib/functions)

# Include directories
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
include_directories("../../utils")
include_directories("../../interfaces")
include_directories("../../../lib/base64/include")

aux_source_directory(. MAIN_SOURCES)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=1 )
find_package(ImageMagick COMPONENTS Magick++ MagickWand MagickCore REQUIRED)
include_directories(${ImageMagick_INCLUDE_DIRS})

add_library({CNAME} SHARED ${MAIN_SOURCES} ${UTIL_SOURCES} ${UTIL_META_SOURCES})
target_link_libraries({CNAME} PRIVATE ${CMAKE_BINARY_DIR}/../../../../lib/libutils.so)
target_link_libraries({CNAME} PRIVATE fmt::fmt)
"""

def generate_cmake(cname, directory):
    opt = pattern.replace("{CNAME}", cname)

    filename = f"{directory}/CMakeLists.txt"

    try:
        os.makedirs(directory, exist_ok=True)
        with open(filename, 'w') as file:
            file.write(opt)
        print(f"Successfully generated CMakeLists.txt for {cname} in {directory}")
    except OSError as e:
        print(f"Failed to create directory or write file: {e}")
import os
def generate_all_cmake(base_directory):
    subdirectories = [name for name in os.listdir(base_directory) if os.path.isdir(os.path.join(base_directory, name))]
    for subdir in subdirectories:
        cname = subdir  # Use the directory name as {CNAME}
        directory = os.path.join(base_directory, subdir)
        generate_cmake(cname, directory)

generate_all_cmake("./")