#include <chrono>
#include <iostream>
#include <thread>

#include "service_discovery/include/service_node.h"
#include "transport/include/block_channel.h"

#include "transport/proto/simple.pb.h"

using Channel = transport::BlockChannel<transport::Message>;

const char id[] = "/test/block1";

void Read() {
  Channel channel(id);
  transport::Message msg;
  while (true) {
    if (channel.Receive(&msg)) {
      std::cout << "msg: " << msg.content() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void Send() {
  service_discovery::ServiceNode node(id, 1883);

  Channel channel(id);
  std::string buf;
  transport::Message msg;
  while (true) {
    std::cin >> buf;
    if (buf[0] == 'q') {
        break;
    }
    msg.set_content(buf);
    if (!channel.Send(msg)) {
      std::cout << "> Send Failed" << std::endl;
    } else {
      std::cout << "> Send Success" << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  if (std::atoi(argv[1]) == 0) {
    Read();
  } else {
    Send();
  }

  return 0;
}
