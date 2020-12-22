#include <iostream>
#include <memory>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "velodyne.pb.h"
#include "velodyne_driver/driver.h"

#include "data_collector/include/data_collector.h"
#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"
#include "transport/include/ipc.h"

#include "proto/lidar.pb.h"

using Channel = transport::BlockChannel<velodyne::VelodyneScan>;

void LocalSaving(velodyne_driver::VelodyneDriver *driver,
                 const lidar::DeviceConf &conf) {
  data_collector::DataCollectParams params;
  params.mode = data_collector::CollectMode::LOCAL_SAVE;
  auto collector = std::shared_ptr<data_collector::DataCollector>(
      new data_collector::DataCollector(conf.data_collecter_ip().c_str(),
                                        conf.data_collecter_port(), &params));
  auto out_topic = conf.conf().base().identifier();

  velodyne::VelodyneScan scan;
  while (true) {
    bool polled_ = driver->poll(&scan);
    if (!polled_) {
      std::cerr << "Velodyne - Failed to poll device." << std::endl;
    } else {
      auto scan_str = scan.SerializeAsString();
      collector->SendData(out_topic.c_str(), scan_str.c_str(), scan_str.size());
    }
  }
}

void Sharing(velodyne_driver::VelodyneDriver *driver,
             const lidar::DeviceConf &conf) {
  auto out_topic = conf.conf().base().identifier();
  Channel channel(out_topic);
  velodyne::VelodyneScan scan;
  while (true) {
    bool polled_ = driver->poll(&scan);
    if (!polled_) {
      std::cerr << "Velodyne - Failed to poll device." << std::endl;
    } else {
      channel.Send(scan);
    }
  }
}

/**
 * The IPC method is not stable enough, so not used right now
 */
void IPCSending(velodyne_driver::VelodyneDriver *driver,
                const lidar::DeviceConf &conf) {
  auto out_topic = conf.rslidar_conf().ipc_topic_name();
  ipc::shm::remove(out_topic.c_str());
  std::shared_ptr<transport::IPC<velodyne::VelodyneScan>> output_;
  output_ = std::make_shared<transport::IPC<velodyne::VelodyneScan>>(out_topic);
  velodyne::VelodyneScan scan;
  while (true) {
    bool polled_ = driver->poll(&scan);
    if (!polled_) {
      std::cerr << "Velodyne - Failed to poll device." << std::endl;
    } else {
      output_->Send(scan);
    }
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

  if (conf.state().has_cut_angle()) {
    conf.mutable_velodyne_conf()->set_cut_angle(conf.state().cut_angle());
  }
  std::shared_ptr<velodyne_driver::VelodyneDriver> driver =
      std::make_shared<velodyne_driver::VelodyneDriver>(conf.velodyne_conf());

  const auto &base_state = conf.state().base();
  const auto &base_conf = conf.conf().base();
  const auto &topic = base_conf.identifier();
  service_discovery::ServiceNode node(topic);

  switch (base_state.mode()) {
  case common::BasicMutableState::LOCAL_SAVING:
    LocalSaving(driver.get(), conf);
    break;
  case common::BasicMutableState::SHARING:
    Sharing(driver.get(), conf);
  default:
    break;
  }

  return 0;
}
