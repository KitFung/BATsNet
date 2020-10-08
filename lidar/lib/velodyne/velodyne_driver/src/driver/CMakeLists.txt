# build the driver node
add_library(velodyne_driver driver.cc)
target_link_libraries(velodyne_driver
  ${libpcap_LIBRARIES}
  velodyne_input
  velodyne_proto
)
target_include_directories(velodyne_driver
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
