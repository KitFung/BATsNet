#include <stdlib.h>

#include <iostream>
#include <memory>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "service_discovery/include/service_node.h"

#include "include/lidar_control.h"

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " pb_conf" << std::endl;
    exit(1);
  }

  lidar::DeviceConf conf;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  const auto &base_conf = conf.conf().base();
  const auto &base_state = conf.state().base();

  std::string server_address = "0.0.0.0:";
  if (base_conf.has_control_service_port()) {
    server_address += base_conf.control_service_port();
  } else {
    server_address += "0";
  }
  lidar::LidarControl controller(conf);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  int port = 0;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(),
                           &port);
  builder.RegisterService(&controller);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[LidarControl] Server listening on "
            << "0.0.0.0:" << port << std::endl;

  service_discovery::ServiceNode node(base_conf.controller_identifier(), 0,
                                      port);
  server->Wait();

  return 0;
}
