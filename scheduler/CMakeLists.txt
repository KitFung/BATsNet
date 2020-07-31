cmake_minimum_required( VERSION 3.9 )
project( scheduler VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# add_compile_options(
#   "-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
#   "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
# )
find_package(Protobuf REQUIRED)
# find_package(Boost 1.40 COMPONENTS algorithm REQUIRED)
# if(Boost_FOUND)
#   include_directories(${Boost_INCLUDE_DIRS})
# endif()

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

add_library(scheduler_common STATIC src/common.cc proto_gen/scheduler.pb.cc)
target_link_libraries(scheduler_common ${Protobuf_LIBRARIES})

add_library(mission_lib SHARED src/mission.cc)
target_link_libraries(mission_lib ${Protobuf_LIBRARIES} stdc++fs scheduler_common)
target_include_directories(mission_lib
  PUBLIC
    ./
  PRIVATE
    src
)

add_library(scheduler_lib SHARED src/scheduler.cc)
target_link_libraries(scheduler_lib ${Protobuf_LIBRARIES} stdc++fs scheduler_common)

add_executable(scheduler src/scheduler_app.cc)
target_link_libraries(scheduler scheduler_lib ${Protobuf_LIBRARIES} stdc++fs)

# file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*.cc)
# file(GLOB CONF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/*)
# add_executable(radar_iwr6843 ${SRC_FILES})
# target_link_libraries(radar_iwr6843 ${Protobuf_LIBRARIES} stdc++fs data_collector)
# target_include_directories(radar_iwr6843 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} data_collector)

# install(DIRECTORY DESTINATION /opt/aiot/drivers/bin)
# install(DIRECTORY conf/ DESTINATION /opt/aiot/drivers/conf/radar)
# install(TARGETS radar_iwr6843 DESTINATION /opt/aiot/drivers/bin)
# install(FILES systemd/radar_iwr6843.service DESTINATION /etc/systemd/system)
