cmake_minimum_required( VERSION 3.9 )
project( livox VERSION 0.0.1 LANGUAGES C CXX)

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
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --python_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

if(INSTALL_DRIVER)
  add_library(livox_lidar_proto STATIC proto/livox.pb.cc)
  target_link_libraries(livox_lidar_proto ${Protobuf_LIBRARIES})
  target_include_directories(livox_lidar_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/proto)
        
  add_library(livox_lidar_lib STATIC src/lds_lidar.cpp src/livox_driver.cpp src/lds.cpp src/ldq.cpp)
  target_link_libraries(livox_lidar_lib livox_lidar_proto livox_sdk_static ${Boost_LIBRARIES})
  target_include_directories(livox_lidar_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/proto ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif(INSTALL_DRIVER)
