#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <experimental/filesystem>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "data_collector/include/data_collector.h"
#include "iwr6843_read.h"
#include "proto_gen/radar.pb.h"

#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"

using Channel = transport::BlockChannel<radar::RadarResult>;
using RawChannel = transport::BlockChannel<radar::RawRadarResult>;

const char *kTTY1 = "/dev/ttyACM0";
const char *kTTY2 = "/dev/ttyACM1";
const char *kTTY3 = "/dev/ttyACM2";
const char *kTTY4 = "/dev/ttyACM3";

// @TODO(kit) dirty code, refactor this code when free
int32_t seq = 0;
radar::MultiResult ress;
radar::MultiRawResult raw_ress;
int pack_per_send = 20;
data_collector::DataCollector *collector = nullptr;
bool collect_mode = false;

void ReadData(radar::Reader_IWR6843 *reader, const std::string &topic) {
  if (collect_mode) {
    while (true) {
      radar::RadarResult res;

      reader->ReadParsedRadarResult(&res);
      auto str = res.SerializeAsString();
      collector->SendData(topic.c_str(), str.c_str(), str.size());
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  } else {
    Channel channel(topic);
    while (true) {
      radar::RadarResult res;
      reader->ReadParsedRadarResult(&res);
      channel.Send(res);
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}
void ReadRawData(radar::Reader_IWR6843 *reader, const std::string &topic) {
  if (collect_mode) {
    while (true) {
      radar::RawRadarResult res;
      reader->ReadRawRadarResult(&res);
      auto str = res.SerializeAsString();
      collector->SendData(topic.c_str(), str.c_str(), str.size());
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  } else {
    RawChannel channel(topic);
    while (true) {
      radar::RawRadarResult res;
      reader->ReadRawRadarResult(&res);
      channel.Send(res);

      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " conf_file" << std::endl;
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY);
  radar::DeviceConf conf;
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  const char *control_port = kTTY1;
  const char *data_port = kTTY2;
  if (!std::experimental::filesystem::exists(control_port)) {
    control_port = kTTY3;
    data_port = kTTY4;
  }

  const auto &base_conf = conf.conf().base();
  const auto &base_state = conf.state().base();
  collect_mode = base_state.mode() == common::BasicMutableState::LOCAL_SAVING;

  data_collector::DataCollectParams params;
  params.mode = data_collector::CollectMode::LOCAL_SAVE;
  if (conf.has_data_collecter_ip()) {
    collector = new data_collector::DataCollector(
        conf.data_collecter_ip().c_str(), conf.data_collecter_port(), &params);
  }

  auto modes = conf.conf().possible_mode();
  const auto &cfg_file = modes.at(conf.state().current_mode());
  radar::Reader_IWR6843 reader(control_port, B115200, data_port, B921600);
  reader.Setup(cfg_file.c_str());

  const auto &topic = base_conf.identifier();
  service_discovery::ServiceNode node(topic);

  std::cout << "Launched Radar\n";
  if (conf.read_parsed()) {
    ReadData(&reader, topic);
  } else {
    ReadRawData(&reader, topic);
  }
  return 0;
}
