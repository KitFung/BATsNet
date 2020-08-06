cmake_minimum_required( VERSION 3.9 )
project( velodyne VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)
# find_package(Boost 1.40 COMPONENTS algorithm REQUIRED)
# if(Boost_FOUND)
#   include_directories(${Boost_INCLUDE_DIRS})
# endif()

# include_directories(${Protobuf_INCLUDE_DIRS})

file(GLOB PB_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*)
if (NOT "${PB_FILE}" STREQUAL "")
  file(REMOVE ${PB_FILE})
endif()

EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       --python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

add_library(velodyne_proto STATIC proto_gen/velodyne.pb.cc)
target_link_libraries(velodyne_proto PUBLIC ${Protobuf_LIBRARIES})
target_include_directories(velodyne_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(velodyne_driver)
add_subdirectory(velodyne_pointcloud)
