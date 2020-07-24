cmake_minimum_required( VERSION 3.9 )
project( radar_iwr6843 VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)
# find_package(Boost 1.40 COMPONENTS algorithm REQUIRED)
# if(Boost_FOUND)
#   include_directories(${Boost_INCLUDE_DIRS})
# endif()

include_directories(${Protobuf_INCLUDE_DIRS})

EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto_gen
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/proto_gen/*.cc)
file(GLOB CONF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/*)
add_executable(radar_iwr6843 ${SRC_FILES})
target_link_libraries(radar_iwr6843 ${Protobuf_LIBRARIES} stdc++fs data_collector)
target_include_directories(radar_iwr6843 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} data_collector)

install(DIRECTORY DESTINATION /opt/aiot/drivers/bin)
install(DIRECTORY conf/ DESTINATION /opt/aiot/drivers/conf/radar)
install(TARGETS radar_iwr6843 DESTINATION /opt/aiot/drivers/bin)
install(FILES systemd/radar_iwr6843.service DESTINATION /etc/systemd/system)
