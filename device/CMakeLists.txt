cmake_minimum_required( VERSION 3.9 )
project( common VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)

# 
# EXEC_PROGRAM(protoc 
#   ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
#        --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
#        ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
# file(GLOB PROTO_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.cc)

add_library(device INTERFACE)
target_link_libraries(device INTERFACE common grpc++ grpc++_reflection)
target_include_directories(device INTERFACE include)
