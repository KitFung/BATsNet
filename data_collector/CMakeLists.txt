cmake_minimum_required( VERSION 3.9 )
project( data_collector VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS EXPORT_MACRO DLL_EXPORT foo.proto)

add_library(data_io STATIC src/data_io.cc)
target_link_libraries(data_io stdc++fs lz4)

add_library(data_collector SHARED src/data_collector.cc)
set_target_properties(data_collector PROPERTIES VERSION ${PROJECT_VERSION})
target_link_libraries(data_collector mosquitto mosquittopp data_io)
target_compile_options (data_collector PRIVATE -Wno-deprecated-declarations)
target_include_directories(data_collector
  PUBLIC
    ./
  PRIVATE
    src
  )

add_library(data_upload_mission_lib STATIC src/data_upload_mission.cc)
target_link_libraries(data_upload_mission_lib data_collector mission_lib)

add_executable(data_upload_mission src/data_upload_mission_app.cc)
target_link_libraries(data_upload_mission data_upload_mission_lib ${Protobuf_LIBRARIES})


install(TARGETS data_collector DESTINATION /usr/local/lib)
