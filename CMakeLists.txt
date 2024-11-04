cmake_minimum_required(VERSION 3.11)

project(shinxbot)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

set(CMAKE_INSTALL_RPATH "./lib")
SET(CMAKE_SKIP_BUILD_RPATH  TRUE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

add_definitions(-DUSE_OPENSSL)
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=1 )
find_package(ImageMagick COMPONENTS Magick++ MagickWand MagickCore REQUIRED)
include_directories(${ImageMagick_INCLUDE_DIRS})
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
include_directories("./src/utils")
include_directories("./src/utils/meta_func")
include_directories("./src/meta_event")
include_directories("./src/bots")
include_directories("./src/interfaces")
include_directories("./lib/base64/include")
include_directories("./lib/cpp-httplib")

aux_source_directory(./src/utils UTIL_SOURCES)
aux_source_directory(./src/utils/meta_func UTIL_META_SOURCES)
aux_source_directory(./src/interfaces INT_SOURCES)
aux_source_directory(./src/bots BOT_SOURCES)
aux_source_directory(./src/meta_event META_SOURCES)
aux_source_directory(./src MAIN_SOURCES)

# Set the mode you want (e.g., Mode1 or Mode2)
set(MODE "main" CACHE STRING "Build mode: main or test")
message(MODE="${MODE}")

# Add all source files in the ./src directory to MAIN_SOURCES
aux_source_directory(./src MAIN_SOURCES)

if(MODE STREQUAL "main" OR MODE STREQUAL "simple")
    list(REMOVE_ITEM MAIN_SOURCES ./src/test.cpp)
endif()

if(MODE STREQUAL "test")
    list(REMOVE_ITEM MAIN_SOURCES ./src/main.cpp)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_link_options("-fuse-ld=lld")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstandalone-debug")
endif()

if(NOT (MODE STREQUAL "simple"))
    add_library(utils SHARED ${UTIL_SOURCES} ${UTIL_META_SOURCES} ${INT_SOURCES})
endif()
link_directories(${CMAKE_SOURCE_DIR}/lib)
add_executable(${PROJECT_NAME} ${BOT_SOURCES} ${META_SOURCES} ${MAIN_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE utils)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ImageMagick_LIBRARIES})

find_package(jsoncpp REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC zip)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -ljsoncpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -pthread)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lm)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -ldl)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "./lib"
)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./lib/")