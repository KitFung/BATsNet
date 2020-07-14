#include <fcntl.h>
#include <iostream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "proto_gen/rslidar.pb.h"
#include "rsdriver.h"

volatile sig_atomic_t flag = 1;

static void my_handler(int sig) { flag = 0; }

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "[Usage] " << argv[0] << " broker_ip broker_port conf_file"
              << std::endl;
    exit(1);
  }
  rslidar::Conf conf;
  int fd = open(argv[3], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  int port = atoi(argv[2]);
  signal(SIGINT, my_handler);

  rslidar::rslidarDriver driver(argv[1], port, conf);

  while (driver.poll()) {
    boost::this_thread::sleep_for(boost::chrono::microseconds(100));
  }
  return 0;
}