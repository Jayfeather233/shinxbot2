cmake_minimum_required(VERSION 3.11)

project(cq_bot)

include_directories("./inc")
include_directories("./interfaces")
include_directories("./functions")
include_directories("./events")

aux_source_directory(./utils UTIL_SOURCES)
aux_source_directory(./ MAIN_SOURCES)

file(GLOB_RECURSE FUNC_SOURCES "./functions/*.cpp")
file(GLOB_RECURSE EVEN_SOURCES "./events/*.cpp")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(${PROJECT_NAME}  ${UTIL_SOURCES} ${MAIN_SOURCES} ${FUNC_SOURCES} ${EVEN_SOURCES})

find_package(jsoncpp REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lcurl)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -ljsoncpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -pthread)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lm)

set(YOU_NEED_X11 1)
set(YOU_NEED_PNG 1)
set(YOU_NEED_JPG 1)

if(${YOU_NEED_X11} EQUAL 1)
    message(STATUS "Looking for X11...")
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${X11_LIBRARIES})
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_display=0)
endif()

if(${YOU_NEED_JPG} EQUAL 1)
    message(STATUS "Looking for libjpg...")
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
    target_link_libraries (${PROJECT_NAME} PUBLIC ${JPEG_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_jpeg=1)
endif()

if(${YOU_NEED_PNG} EQUAL 1)
    message(STATUS "Looking for libpng...")
    find_package(PNG REQUIRED)
    include_directories(${PNG_INCLUDE_DIR})
    target_link_libraries (${PROJECT_NAME} PUBLIC ${PNG_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PUBLIC cimg_use_png=1)
endif()