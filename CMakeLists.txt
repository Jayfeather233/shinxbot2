cmake_minimum_required(VERSION 3.11)

project(cq_bot)

include_directories("./inc")
include_directories("./interfaces")
include_directories("./functions")


aux_source_directory(. SOURCES)
aux_source_directory(./utils SOURCES)
aux_source_directory(./functions/getJapaneseImage SOURCES)
aux_source_directory(./functions/auto114 SOURCES)
aux_source_directory(./functions/621 SOURCES)
aux_source_directory(./functions/hhsh SOURCES)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

add_executable(${PROJECT_NAME}  ${SOURCES})


find_package(jsoncpp REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${JSONCPP_LIBRARIES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lcurl)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -lboost_system)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -pthread)