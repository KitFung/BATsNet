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

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen)
EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       --python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

add_library(rslidar_proto STATIC proto_gen/rslidar.pb.cc)
target_link_libraries(rslidar_proto ${Protobuf_LIBRARIES})
target_include_directories(rslidar_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen)
       
add_library(rslidar_lib STATIC src/input.cc src/rsdriver.cpp)
target_link_libraries(rslidar_lib rslidar_proto ${Boost_LIBRARIES})
target_include_directories(rslidar_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen ${CMAKE_CURRENT_SOURCE_DIR}/src)

# install(DIRECTORY DESTINATION /opt/aiot/drivers/bin)
# install(DIRECTORY conf/ DESTINATION /opt/aiot/drivers/conf/rslidar)
# install(TARGETS rslidar DESTINATION /opt/aiot/drivers/bin)
# install(FILES systemd/rslidar.service DESTINATION /etc/systemd/system)
