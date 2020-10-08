cmake_minimum_required( VERSION 3.9 )
project( camera VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen)
EXEC_PROGRAM(protoc 
  ARGS --grpc_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       -I=${GLOBAL_HEADER}/common/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
# EXEC_PROGRAM(protoc 
#   ARGS --grpc_python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
#       -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
#       -I=${GLOBAL_HEADER}/common/proto
#       --python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
#       --plugin=protoc-gen-grpc_python=`which grpc_python_plugin`
#       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
file(GLOB PROTO_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*.cc)

include_directories(${GLOBAL_HEADER})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(camera_proto STATIC ${PROTO_SRC_FILES})
target_link_libraries(camera_proto ${Protobuf_LIBRARIES} common)
target_include_directories(camera_proto PUBLIC proto_gen/)

add_executable(camera src/camera.cc)
target_link_libraries(camera camera_proto ${Protobuf_LIBRARIES} service_node)

add_library(camera_control_lib STATIC src/camera_control.cc src/trafi_one_control.cc)
target_link_libraries(camera_control_lib camera_proto device grpc++ grpc++_reflection)

add_executable(camera_controller src/controller.cc)
target_link_libraries(camera_controller camera_control_lib service_node)