cmake_minimum_required( VERSION 3.9 )
project( rslidar VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)
find_package(Boost 1.40 COMPONENTS thread chrono REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

add_library(rslidar_lib SHARED src/input.cc src/rsdriver.cpp proto_gen/rslidar.pb.cc)
target_link_libraries(rslidar_lib ${Protobuf_LIBRARIES} data_collector)
target_include_directories(rslidar_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} data_collector)


add_executable(rslidar src/main.cc)
target_link_libraries(rslidar rslidar_lib ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} data_collector)
