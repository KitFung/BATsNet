cmake_minimum_required(VERSION 3.0)

project(service_discovery_pybind)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(pybind11 REQUIRED)

pybind11_add_module(service_discovery_py src/service_discovery_py.cpp)
target_link_libraries(service_discovery_py PRIVATE service_discovery ssl)
