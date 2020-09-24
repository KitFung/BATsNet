#include <chrono>
#include <iostream>
#include <thread>

#include "include/service_node.h"

int main(int argc, char *argv[]) {
  service_discovery::ServiceNode node("/casdasdc", 12345);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}