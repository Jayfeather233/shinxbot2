cmake_minimum_required(VERSION 3.11)

project(shinxbot)

add_definitions(-DUSE_OPENSSL)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories("./src/utils")
include_directories("./src/bots")
include_directories("./src/interfaces")
include_directories("./src/functions")
include_directories("./src/events")
include_directories("./lib/base64/include")

aux_source_directory(./src/utils UTIL_SOURCES)
aux_source_directory(./src/interfaces INT_SOURCES)
aux_source_directory(./src/bots BOT_SOURCES)
aux_source_directory(./src MAIN_SOURCES)

# Set the mode you want (e.g., Mode1 or Mode2)
set(MODE "Mode" CACHE STRING "Build mode: main or test")

# Add all source files in the ./src directory to MAIN_SOURCES
aux_source_directory(./src MAIN_SOURCES)

# Exclude A.cpp in Mode1
if(MODE STREQUAL "main")
    list(REMOVE_ITEM MAIN_SOURCES ./src/test.cpp)
endif()

# Exclude B.cpp in Mode2
if(MODE STREQUAL "test")
    list(REMOVE_ITEM MAIN_SOURCES ./src/main.cpp)
endif()

file(GLOB_RECURSE FUNC_SOURCES "./src/functions/*.cpp")
file(GLOB_RECURSE EVEN_SOURCES "./src/events/*.cpp")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_link_options("-fuse-ld=lld")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstandalone-debug")
endif()

add_executable(${PROJECT_NAME}  ${UTIL_SOURCES} ${INT_SOURCES} ${BOT_SOURCES} ${MAIN_SOURCES} ${FUNC_SOURCES} ${EVEN_SOURCES})

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++ MagickCore REQUIRED)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ImageMagick_LIBRARIES})
# find_package(ImageMagick COMPONENTS MagickCore REQUIRED)
# include_directories(${ImageMagick_INCLUDE_DIRS})
# target_link_libraries(${PROJECT_NAME} PUBLIC ${ImageMagick_LIBRARIES})

find_package(jsoncpp REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC zip)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lcurl)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -ljsoncpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -pthread)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lm)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})

set(YOU_NEED_X11 0)
set(YOU_NEED_PNG 1)
set(YOU_NEED_JPG 1)
set(YOU_NEED_TIFF 1)
set(YOU_NEED_OPENMP 1)

if(${YOU_NEED_X11} EQUAL 1)
    message(STATUS "Looking for X11...")
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${X11_LIBRARIES})
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_display=0)
endif()

# if(${YOU_NEED_JPG} EQUAL 1)
#     message(STATUS "Looking for libjpg...")
#     find_package(JPEG REQUIRED)
#     include_directories(${JPEG_INCLUDE_DIR})
#     target_link_libraries (${PROJECT_NAME} PUBLIC ${JPEG_LIBRARY})
#     target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_jpeg=1)
# endif()

# if(${YOU_NEED_TIFF} EQUAL 1)
#     message(STATUS "Looking for libtiff...")
#     find_package(TIFF REQUIRED)
#     include_directories(${TIFF_INCLUDE_DIR})
#     target_link_libraries (${PROJECT_NAME} PUBLIC ${TIFF_LIBRARY})
#     target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_tiff=1)
# endif()

# if(${YOU_NEED_PNG} EQUAL 1)
#     message(STATUS "Looking for libpng...")
#     find_package(PNG REQUIRED)
#     include_directories(${PNG_INCLUDE_DIR})
#     target_link_libraries (${PROJECT_NAME} PUBLIC ${PNG_LIBRARY})
#     target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_png=1)
# endif()

# if(${YOU_NEED_OPENMP} EQUAL 1)
#     message(STATUS "Looking for OpenMP...")
#     find_package(OpenMP REQUIRED)
#     target_compile_options(${PROJECT_NAME} PUBLIC ${OpenMP_CXX_FLAGS})
#     target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenMP_CXX_LIBRARIES})
#     target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_openmp=1)
# endif()