#include <stdlib.h>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <memory>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "service_discovery/include/service_node.h"

#include "proto/camera.pb.h"

void LocalSaving(const camera::DeviceConf &conf) {
  const auto &in_stream = conf.stream_path();
  const auto &save_pattern = conf.save_pattern();
  std::string bin = "ffmpeg";
  std::string cmd = "ffmpeg -i ";
  cmd += in_stream;
  cmd += " -acodec copy -b 900k -vcodec copy ";
  cmd += "-f segment -strftime 1 -segment_time 300 -segment_format mp4 ";
  cmd += save_pattern;
  std::cout << cmd << std::endl;

  std::vector<std::string> strs;
  boost::split(strs, cmd, boost::is_any_of(" "), boost::token_compress_on);

  int argc = strs.size();
  const char **argv = new const char *[argc + 1];
  for (int i = 0; i < argc; ++i) {
    argv[i] = strs[i].c_str();
  }
  argv[argc] = nullptr;
  while (true) {
    int ret = execvp("ffmpeg", const_cast<char **>(argv));
    if (ret == -1) {
      perror("Error while Saving: ");
      exit(EXIT_FAILURE);
    }
  }
}

void Sharing(const camera::DeviceConf &conf) {
  const auto &in_stream = conf.stream_path();
  const auto &out_stream = conf.output_stream_path();
  std::string cmd = "ffmpeg -flags low_delay -fflags nobuffer -i ";
  cmd += in_stream;
  cmd += " -codec copy -f rtsp ";
  cmd += out_stream;
  std::cout << cmd << std::endl;

  std::vector<std::string> strs;
  boost::split(strs, cmd, boost::is_any_of(" "), boost::token_compress_on);

  int argc = strs.size();
  const char **argv = new const char *[argc + 1];
  for (int i = 0; i < argc; ++i) {
    argv[i] = strs[i].c_str();
  }
  int ret = execvp("ffmpeg", const_cast<char **>(argv));
  if (ret == -1) {
    perror("Error while Sharing: ");
    exit(EXIT_FAILURE);
  }
}

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
  service_discovery::ServiceNode node(base_conf.identifier());

  switch (base_state.mode()) {
  case common::BasicMutableState::LOCAL_SAVING:
    LocalSaving(conf);
    break;
  case common::BasicMutableState::SHARING:
    Sharing(conf);
    break;
  default:
    break;
  }

  return 0;
}
