cmake_minimum_required( VERSION 3.9 )
project(batsnet)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(third_party)

add_subdirectory(common)
add_subdirectory(service_discovery)
add_subdirectory(transport)

option(IN_FOG "Enable compile the driver in fog" OFF) #OFF by default
if(IN_FOG)
    add_subdirectory(device)
    add_subdirectory(scheduler)
    add_subdirectory(data_collector)

    add_subdirectory(radar_iwr6843)
    add_subdirectory(camera)
    add_subdirectory(lidar)

    install(FILES env.sh DESTINATION /opt/aiot/drivers/)
else()
    add_subdirectory(server_side)
endif(IN_FOG)

