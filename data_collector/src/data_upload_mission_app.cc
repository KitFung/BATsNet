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

#include "include/data_upload_mission.h"

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "[Usage] " << argv[0] << " mission_conf host port"
              << std::endl;
    exit(1);
  }

  scheduler::MissionSetting setting;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &setting)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  data_collector::DataUploadMission mission(setting.name(), setting, argv[2],
                                            atoi(argv[3]));
  mission.Run();
  return 0;
}