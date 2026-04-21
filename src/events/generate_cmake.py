import argparse
import os
import re

pattern = """
cmake_minimum_required(VERSION 3.10)

project({CNAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH "./lib")

# Set the output directory for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../../../lib/events)

SET(CMAKE_SKIP_BUILD_RPATH  TRUE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# Include directories
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
include_directories("../../utils")
include_directories("../../utils/meta_func")
include_directories("../../interfaces")
include_directories("../../../lib/base64/include")
include_directories("../../../lib/cpp-httplib")

aux_source_directory(. MAIN_SOURCES)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=1 )
find_package(ImageMagick COMPONENTS Magick++ MagickWand MagickCore REQUIRED)
include_directories(${ImageMagick_INCLUDE_DIRS})

add_library({CNAME} SHARED ${MAIN_SOURCES} ${UTIL_SOURCES} ${UTIL_META_SOURCES})
target_link_libraries({CNAME} PRIVATE ${CMAKE_BINARY_DIR}/../../../../lib/libutils.so)
target_link_libraries({CNAME} PRIVATE fmt::fmt)
set_target_properties({CNAME} PROPERTIES
    INSTALL_RPATH "./lib"
)
set_target_properties({CNAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./lib/")

"""

header_template = """#pragma once

#include \"eventprocess.h\"

class {CLASS_NAME} : public eventprocess {{
public:
    bool check(bot *p, Json::Value J) override;
    void process(bot *p, Json::Value J) override;
}};

DECLARE_FACTORY_FUNCTIONS_HEADER
"""

cpp_template = """#include \"{HEADER_FILE}\"

#include \"utils.h\"

bool {CLASS_NAME}::check(bot *p, Json::Value J)
{{
    (void)p;
    return J.isMember(\"post_type\") &&
           J[\"post_type\"].asString() == \"notice\";
}}

void {CLASS_NAME}::process(bot *p, Json::Value J)
{{
    (void)J;
    p->setlog(LOG::INFO, \"{MODULE} event template handled one event\");
}}

DECLARE_FACTORY_FUNCTIONS({CLASS_NAME})
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


def module_to_class(module_name):
    tokens = [t for t in re.split(r"[^0-9A-Za-z]+", module_name) if t]
    class_name = "".join(t[:1].upper() + t[1:] for t in tokens)
    if not class_name:
        class_name = "EventModule"
    if class_name[0].isdigit():
        class_name = "M" + class_name
    return class_name


def init_module(module_name, base_directory="./"):
    module_dir = os.path.join(base_directory, module_name)
    os.makedirs(module_dir, exist_ok=True)

    class_name = module_to_class(module_name)
    header_file = f"{module_name}.h"
    cpp_file = f"{module_name}.cpp"

    generate_cmake(module_name, module_dir)

    header_path = os.path.join(module_dir, header_file)
    cpp_path = os.path.join(module_dir, cpp_file)

    if not os.path.exists(header_path):
        with open(header_path, "w") as f:
            f.write(header_template.replace("{CLASS_NAME}", class_name))

    if not os.path.exists(cpp_path):
        content = (
            cpp_template.replace("{CLASS_NAME}", class_name)
            .replace("{HEADER_FILE}", header_file)
            .replace("{MODULE}", module_name)
        )
        with open(cpp_path, "w") as f:
            f.write(content)

    print(f"Initialized event module skeleton at {module_dir}")


def generate_all_cmake(base_directory):
    subdirectories = [name for name in os.listdir(base_directory) if os.path.isdir(os.path.join(base_directory, name))]
    for subdir in subdirectories:
        cname = subdir  # Use the directory name as {CNAME}
        directory = os.path.join(base_directory, subdir)
        generate_cmake(cname, directory)


def main():
    parser = argparse.ArgumentParser(
        description="Generate event CMakeLists or initialize a module skeleton."
    )
    parser.add_argument(
        "--init",
        metavar="MODULE",
        help="Initialize a new event module skeleton under current directory.",
    )
    args = parser.parse_args()

    if args.init:
        init_module(args.init, "./")
    else:
        generate_all_cmake("./")


if __name__ == "__main__":
    main()