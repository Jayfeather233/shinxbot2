cmake_minimum_required(VERSION 3.11)

project(cq_bot)

include_directories("./inc")
include_directories("./interfaces")
include_directories("./functions")
include_directories("./events")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("/usr/local/include/opencv4/opencv2")


file(GLOB_RECURSE SOURCES "./functions/*.cpp")
file(GLOB_RECURSE SOURCES "./events/*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
aux_source_directory(./utils SOURCES)
aux_source_directory(./ SOURCES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(${PROJECT_NAME}  ${SOURCES})


find_package(jsoncpp REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lcurl)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lboost_system)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -pthread)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})