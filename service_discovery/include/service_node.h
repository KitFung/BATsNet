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
  // If local port is zero, it mean the service is accessable in remote directly
  // Otherwise, it mean it need the remote to proxy the packet to local port
  ServiceNode(const std::string &identifier, const int remote_port = 0,
              const int local_port = 0);

  // For register video stream
  ServiceNode(const std::string &identifier, const std::string &path);
  ~ServiceNode();

private:
  std::string SendUdp(const std::string &msg) const;
  // The IP is the BATs IP, so must get from BATS
  std::string RetreiveServiceIP() const;
  // The MQTT port is seted in GLOBAL ENV, so get from env
  int RetreiveEnvPort() const;

  // Register itself to etcd. (identifier, IP:port)
  void Register();
  void RenewRegister();
  void ValidProxyAlive();

  // Loop, Renew it register frequently
  void InnerLoop();

  std::string identifier_;
  std::string val_;
  std::thread inner_loop_;
  std::shared_ptr<etcd::Client> etcd_;
  std::shared_ptr<etcd::Client> local_etcd_;
  int service_port_ = 0;
  int local_port_ = 0;
  int loop_interval_s_ = 2;
  std::atomic<int> registered_ = {0};
  int running_ = true;
};
} // namespace service_discovery
