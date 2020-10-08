#include <fcntl.h>
#include <iostream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "rsdriver.h"
#include "rslidar.pb.h"

#include "data_collector/include/data_collector.h"
#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"

#include "proto_gen/lidar.pb.h"

using Channel = transport::BlockChannel<rslidar::Scan>;

volatile sig_atomic_t flag = 1;

static void my_handler(int sig) { flag = 0; }

void LocalSaving(rslidar::rslidarDriver *driver,
                 const lidar::DeviceConf &conf) {
  auto out_topic = conf.conf().base().identifier();
  data_collector::DataCollectParams params;
  params.mode = data_collector::CollectMode::LOCAL_SAVE;
  auto collector_ = boost::shared_ptr<data_collector::DataCollector>(
      new data_collector::DataCollector(conf.data_collecter_ip().c_str(),
                                        conf.data_collecter_port(), &params));

  rslidar::Scan scan;
  while (driver->poll(&scan)) {
    auto data_str = scan.SerializeAsString();
    bool flag = collector_->SendData(out_topic.c_str(), data_str.c_str(),
                                     data_str.size());

    boost::this_thread::sleep_for(boost::chrono::microseconds(100));
  }
}

void Sharing(rslidar::rslidarDriver *driver, const lidar::DeviceConf &conf) {
  auto out_topic = conf.rslidar_conf().ipc_topic_name();
  Channel channel(out_topic);

  rslidar::Scan scan;
  while (driver->poll(&scan)) {
    channel.Send(scan);
    boost::this_thread::sleep_for(boost::chrono::microseconds(100));
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

  if (conf.state().has_cut_angle()) {
    conf.mutable_rslidar_conf()->set_cut_angle(conf.state().cut_angle());
  }
  rslidar::rslidarDriver driver(conf.rslidar_conf());

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
