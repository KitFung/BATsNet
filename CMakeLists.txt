cmake_minimum_required( VERSION 3.9 )
project(batsnet)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(third_party)

add_subdirectory(common)
add_subdirectory(transport)
add_subdirectory(service_discovery)
add_subdirectory(device)
add_subdirectory(scheduler)
add_subdirectory(data_collector)

# add_subdirectory(rslidar)
add_subdirectory(radar_iwr6843)
add_subdirectory(camera)

# add_subdirectory(velodyne)
# add_subdirectory(server_side)

install(FILES env.sh DESTINATION /opt/aiot/drivers/)
