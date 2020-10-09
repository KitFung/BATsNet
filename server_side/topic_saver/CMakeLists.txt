cmake_minimum_required( VERSION 3.9 )
project( topic_saver VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# find_package(Protobuf REQUIRED)
# find_package(Boost 1.40 COMPONENTS algorithm REQUIRED)
# if(Boost_FOUND)
#   include_directories(${Boost_INCLUDE_DIRS})
# endif()

# include_directories(${Protobuf_INCLUDE_DIRS})

# EXEC_PROGRAM(protoc 
#   ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
#        --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
#        ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(topic_saver_lib SHARED src/bin_writer.cc src/topic_saver.cc)
target_link_libraries(topic_saver_lib stdc++fs mosquitto)

add_executable(topic_saver src/main.cc)
target_link_libraries(topic_saver stdc++fs topic_saver_lib)
