add_library(velodyne_input input.cc)
target_link_libraries(velodyne_input
  ${libpcap_LIBRARIES}
  velodyne_proto
)
