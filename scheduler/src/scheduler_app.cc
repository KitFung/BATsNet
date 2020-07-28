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

#include "include/scheduler.h"
#include "proto_gen/scheduler.pb.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " conf_file" << std::endl;
    exit(1);
  }
  scheduler::SchedulerSetting setting;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &setting)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  scheduler::Scheduler scheduler(setting);
  scheduler.Setup();
  scheduler.Run();
  return 0;
}