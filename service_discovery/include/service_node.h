#pragma once

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "etcd/Client.hpp"

#include "include/common.h"

namespace service_discovery {
/**
 *  Let the service able to be discover by other node
 */
class ServiceNode {
public:
  ServiceNode(const std::string &identifier, const int port = 0);
  ~ServiceNode();

private:
  // The IP is the BATs IP, so must get from BATS
  std::string RetreiveBrokerIP() const;
  // The MQTT port is seted in GLOBAL ENV, so get from env
  int RetreiveEnvPort() const;

  // Register itself to etcd. (identifier, IP:port)
  void Register();
  void RenewRegister();

  // Loop, Renew it register frequently
  void InnerLoop();

  std::string identifier_;
  std::string val_;
  std::thread inner_loop_;
  std::shared_ptr<etcd::Client> etcd_;
  int cport_ = 0;
  int loop_interval_s_ = 2;
  std::atomic<int> registered_ = {0};
  int running_ = true;
};
} // namespace service_discovery
