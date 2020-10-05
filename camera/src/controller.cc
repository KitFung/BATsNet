#include <stdlib.h>

#include <iostream>
#include <memory>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "service_discovery/include/service_node.h"

#include "include/camera_control.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " pb_conf" << std::endl;
    exit(1);
  }

  camera::DeviceConf conf;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  const auto &base_conf = conf.conf().base();
  const auto &base_state = conf.state().base();
  service_discovery::ServiceNode node(base_conf.controller_identifier());

  camera::CameraControl controller(conf.conf(), conf.state());

  while (true)
    ;
  return 0;
}