cmake_minimum_required( VERSION 3.9 )
project(batsnet)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR})

option(IN_FOG "Enable compile the driver in fog" OFF) #OFF by default
option(IN_BAT "Install for BATs" OFF) #OFF by default
option(INSTALL_BASE_DIR "" /opt/aiot)
if(IN_FOG)
    set(BATSNET_LIB_INSTALL_DIR /usr/local/lib/batsnet)
    set(BATSNET_BIN_INSTALL_DIR ${INSTALL_BASE_DIR}/bin)
    set(BATSNET_CONF_INSTALL_DIR ${INSTALL_BASE_DIR}/conf)
    install(DIRECTORY DESTINATION ${INSTALL_BASE_DIR})
    set(CHMOD_CMD "execute_process(COMMAND /bin/chmod ARGS 0777 ${INSTALL_BASE_DIR})")
    install(CODE ${CHMOD_CMD})
    install(DIRECTORY DESTINATION ${BATSNET_LIB_INSTALL_DIR})
    install(DIRECTORY DESTINATION ${BATSNET_BIN_INSTALL_DIR})
    install(DIRECTORY DESTINATION ${BATSNET_CONF_INSTALL_DIR})

    add_subdirectory(third_party)

    add_subdirectory(common)
    add_subdirectory(service_discovery)
    add_subdirectory(transport)

    add_subdirectory(device)
    add_subdirectory(scheduler)
    add_subdirectory(data_collector)

    add_subdirectory(radar_iwr6843)
    add_subdirectory(camera)
    add_subdirectory(lidar)

    add_subdirectory(fog_side)
elseif(IN_BAT)
    add_subdirectory(bat_side)
else()
    add_subdirectory(server_side)
endif(IN_FOG)

