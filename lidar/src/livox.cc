#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <thread>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "livox.pb.h"
#include "livox_driver.h"

#include "data_collector/include/data_collector.h"
#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"

#include "proto/lidar.pb.h"

using Channel = transport::BlockChannel<livox::PointCloud>;

volatile sig_atomic_t flag = 1;

static void my_handler(int sig) { flag = 0; }

void LocalSaving(livox::livoxDriver *driver, const lidar::DeviceConf &conf) {
  auto out_topic = conf.conf().base().identifier();
  data_collector::DataCollectParams params;
  params.mode = data_collector::CollectMode::LOCAL_SAVE;
  auto collector_ = std::shared_ptr<data_collector::DataCollector>(
      new data_collector::DataCollector(conf.data_collecter_ip().c_str(),
                                        conf.data_collecter_port(), &params));

  livox::PointCloud cloud;
  while (driver->poll(&cloud)) {
    if (cloud.point_size() > 0) {
      auto data_str = cloud.SerializeAsString();
      std::cout << cloud.stamp() << " : " << cloud.point_size() << std::endl;
      bool flag = collector_->SendData(out_topic.c_str(), data_str.c_str(),
                                       data_str.size());
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void Sharing(livox::livoxDriver *driver, const lidar::DeviceConf &conf) {
  auto out_topic = conf.livox_conf().ipc_topic_name();
  Channel channel(out_topic);

  livox::PointCloud cloud;
  while (driver->poll(&cloud)) {
    if (cloud.point_size() > 0) {
      channel.Send(cloud);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " conf_file" << std::endl;
    exit(1);
  }
  lidar::DeviceConf conf;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  signal(SIGINT, my_handler);
  livox::livoxDriver driver(conf.livox_conf());

  const auto &base_state = conf.state().base();

  const auto &base_conf = conf.conf().base();
  const auto &topic = base_conf.identifier();

  switch (base_state.mode()) {
  case common::BasicMutableState::LOCAL_SAVING:
    LocalSaving(&driver, conf);
    break;
  case common::BasicMutableState::SHARING:
    Sharing(&driver, conf);
  default:
    break;
  }

  return 0;
}
