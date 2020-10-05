cmake_minimum_required( VERSION 3.9 )
project( common VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen)
EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
file(GLOB PROTO_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*.cc)

include_directories(proto_gen/)

add_library(common SHARED ${PROTO_SRC_FILES})
target_link_libraries(common ${Protobuf_LIBRARIES})
target_include_directories(common PUBLIC proto_gen/)
