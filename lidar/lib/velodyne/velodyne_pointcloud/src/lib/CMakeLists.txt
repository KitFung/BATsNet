add_library(velodyne_rawdata SHARED rawdata.cc calibration.cc)
target_link_libraries(velodyne_rawdata 
                      ${YAML_CPP_LIBRARIES} velodyne_proto)
