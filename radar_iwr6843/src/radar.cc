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

const char *kTTY1 = "/dev/ttyACM0";
const char *kTTY2 = "/dev/ttyACM1";
const char *kTTY3 = "/dev/ttyACM2";
const char *kTTY4 = "/dev/ttyACM3";

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "[Usage] " << argv[0] << " broker_ip broker_port conf_file"
              << std::endl;
    exit(1);
  }
  int port = atoi(argv[2]);
  radar::Conf conf;
  int fd = open(argv[3], O_RDONLY);
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
  std::shared_ptr<data_collector::DataCollector> collector(
      new data_collector::DataCollector(argv[1], port));

  radar::Reader_IWR6843 reader(control_port, B115200, data_port, B921600);
  reader.Setup(conf.conf().c_str());

  int32_t seq = 0;
  radar::MultiResult ress;
  radar::MultiRawResult raw_ress;
  int pack_per_send = 20;
  while (true) {
    if (conf.read_parsed()) {
      radar::RadarResult res;
      reader.ReadParsedRadarResult(&res);
      auto str = res.SerializeAsString();
      collector->SendData(conf.write_topic().c_str(), str.c_str(), str.size());

      // auto res = ress.add_parsed_result();

      // reader.ReadParsedRadarResult(res);
      // if (ress.parsed_result_size() >= pack_per_send) {
      //   seq++;
      //   ress.set_seq(seq);
      //   ress.set_timestamp(res->timestamp());
      //   collector->SendData(conf.write_topic().c_str(),
      //                       ress.SerializeAsString().c_str());
      //   ress.Clear();
      // }
    } else {
      radar::RawRadarResult res;
      reader.ReadRawRadarResult(&res);
      auto str = res.SerializeAsString();
      collector->SendData(conf.write_topic().c_str(), str.c_str(), str.size());

      // auto res = raw_ress.add_result();
      // reader.ReadRawRadarResult(res);
      // if (raw_ress.result_size() >= pack_per_send) {
      //   seq++;
      //   raw_ress.set_seq(seq);
      //   raw_ress.set_timestamp(res->timestamp());
      //   collector->SendData(conf.write_topic().c_str(),
      //                       raw_ress.SerializeAsString().c_str());
      //   raw_ress.Clear();
      // }
    }
    std::cout << "Read" << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  return 0;
}