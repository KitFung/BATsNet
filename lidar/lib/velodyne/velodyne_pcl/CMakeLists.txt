cmake_minimum_required(VERSION 3.5)
project(velodyne_pcl)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS pcl_ros
)

include_directories(
  include
)
