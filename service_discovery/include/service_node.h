#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "third_party/etcd-cpp-apiv3/etcd/Client.hpp"

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
  std::string RetreiveBatsIP() const;
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
  int loop_interval_s_ = 5;
};
} // namespace service_discovery
