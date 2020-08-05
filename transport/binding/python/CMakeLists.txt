cmake_minimum_required(VERSION 3.0)

project(transport_pybind)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(pybind11 REQUIRED)

pybind11_add_module(ipc_py src/ipc_py.cpp)
target_link_libraries(ipc_py PRIVATE transport)
