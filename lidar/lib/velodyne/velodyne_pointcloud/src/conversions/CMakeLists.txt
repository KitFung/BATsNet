add_library(velodyne_transform_lib SHARED transform.cc pointcloudXYZIRT.cc)
target_link_libraries(velodyne_transform_lib
    PUBLIC ${YAML_CPP_LIBRARIES} transport velodyne_proto
    PRIVATE velodyne_rawdata)

add_executable(velodyne_pointcloud transform_node.cc)
target_link_libraries(velodyne_pointcloud velodyne_transform_lib velodyne_proto)
