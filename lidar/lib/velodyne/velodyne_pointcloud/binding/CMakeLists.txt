cmake_minimum_required(VERSION 3.0)

project(velodyne_pybind)

find_package(pybind11 REQUIRED)

pybind11_add_module(velodyne_py src/velodyne_py.cc)
target_link_libraries(velodyne_py PRIVATE velodyne_transform_lib transport ssl)
