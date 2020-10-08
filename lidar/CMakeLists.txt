cmake_minimum_required( VERSION 3.9 )
project( lidar VERSION 0.0.1 LANGUAGES C CXX)

add_subdirectory(lib)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen)
EXEC_PROGRAM(protoc 
  ARGS --grpc_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       -I=${GLOBAL_HEADER}/common/proto
       -I=${CMAKE_CURRENT_SOURCE_DIR}/lib/velodyne/proto
       -I=${CMAKE_CURRENT_SOURCE_DIR}/lib/rslidar/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
file(GLOB PROTO_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*.cc)

add_library(lidar_proto STATIC ${PROTO_SRC_FILES})
target_link_libraries(lidar_proto ${Protobuf_LIBRARIES}
    common rslidar_proto velodyne_proto)
  
add_executable(rslidar src/rslidar.cc)
target_link_libraries(rslidar rslidar_lib 
    lidar_proto ${Protobuf_LIBRARIES} data_collector transport service_node)

add_executable(velodyne src/velodyne.cc)
target_link_libraries(velodyne velodyne_driver 
    lidar_proto ${Protobuf_LIBRARIES} data_collector transport service_node)

add_library(lidar_control_lib STATIC src/lidar_control.cc src/rslidar_control.cc src/velodyne_control.cc)
target_link_libraries(lidar_control_lib device lidar_proto)

add_executable(lidar_controller src/controller.cc)
target_link_libraries(lidar_controller lidar_control_lib service_node)