cmake_minimum_required( VERSION 3.9 )
project(batsnet)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(data_collecter)
add_subdirectory(rslidar)
