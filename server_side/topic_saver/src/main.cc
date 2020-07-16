#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "topic_saver.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "[Usage] " << argv[0] << " broker_ip broker_port" << std::endl;
    exit(1);
  }

  mosquitto_lib_init();

  const char *host = argv[1];
  int port = atoi(argv[2]);

  std::shared_ptr<topic_saver::TopicSaver> saver =
      std::make_shared<topic_saver::AllTopicSaver>(host, port, "/#",
                                                   "/home/kit/test");
  saver->RunUntilEnd();
  mosquitto_lib_cleanup();
  return 0;
}