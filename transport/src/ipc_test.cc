#include <atomic>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>

#include "transport/include/ipc.h"
#include "transport/proto/simple.pb.h"

namespace {

char name__[] = "ipc-chat";
char quit__[] = "q";
char id__[] = "c";

std::size_t calc_unique_id() {
  static ipc::shm::handle g_shm{"__CHAT_ACC_STORAGE__",
                                sizeof(std::atomic<std::size_t>)};
  return static_cast<std::atomic<std::size_t> *>(g_shm.get())
      ->fetch_add(1, std::memory_order_relaxed);
}

} // namespace

int main() {
  std::string buf, id = id__ + std::to_string(calc_unique_id());
  std::regex reg{"(c\\d+)> (.*)"};

  std::shared_ptr<transport::Transport<transport::Message>> cc(
      new transport::IPC<transport::Message>(name__));

  std::thread r{[&id, &reg] {
    std::shared_ptr<transport::Transport<transport::Message>> cc(
        new transport::IPC<transport::Message>(name__));
    std::cout << id << " is ready." << std::endl;
    while (1) {
      transport::Message msg;
      cc->Receive(&msg);
      std::string dat = msg.content();
      std::smatch mid;
      if (std::regex_match(dat, mid, reg)) {
        if (mid.str(1) == id) {
          if (mid.str(2) == quit__) {
            std::cout << "receiver quit..." << std::endl;
            return;
          }
          continue;
        }
      }
      std::cout << dat << std::endl;
    }
  }};

  for (/*int i = 1*/;; /*++i*/) {
    std::cin >> buf;
    //        std::cout << "[" << i << "]" << std::endl;
    transport::Message msg;
    msg.set_content(id + "> " + buf);
    cc->Send(msg);
    if (buf == quit__)
      break;
  }

  r.join();
  return 0;
}
