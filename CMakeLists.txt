cmake_minimum_required( VERSION 3.9 )
project(batsnet)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(data_collector)
add_subdirectory(rslidar)
add_subdirectory(radar_iwr6843)
add_subdirectory(server_side)

install(FILES env.sh DESTINATION /opt/drivers/)
