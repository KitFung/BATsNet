# build the driver node
# add_library(velodyne_driver velodyne_driver.cc driver.cc)
add_executable(velodyne_driver velodyne_driver.cc driver.cc)
target_link_libraries(velodyne_driver
  ${libpcap_LIBRARIES}
  velodyne_input
  velodyne_proto
  transport
  data_collector
)
