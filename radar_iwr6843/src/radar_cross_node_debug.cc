#include <chrono>
#include <iostream>
#include <thread>

#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"

#include "proto/radar.pb.h"

using Channel = transport::BlockChannel<radar::RadarResult>;

const char id[] = "/radar/iwr6843/raw/1/lab/test1";

void Read() {
  Channel channel(id);
  radar::RadarResult msg;
  while (true) {
    if (channel.Receive(&msg)) {
      std::cout << "msg: " << msg.DebugString() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

int main(int argc, char *argv[]) {
  Read();

  return 0;
}
