#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <experimental/filesystem>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "proto/velodyne.pb.h"
#include "velodyne_pointcloud/transform.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " conf_file" << std::endl;
    exit(1);
  }
  velodyne::VelodynePointCloudConf conf;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  velodyne_pointcloud::Transform transform(conf);
  transform.Start();
  return 0;
}
