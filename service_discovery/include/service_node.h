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
 * ServiceNode take charge to registry the service to the BAT service register
 * Other node cannot see this service if not register.
 */
class ServiceNode {
public:
  /**
   * identifier: The service name
   *
   * remote_port: If the local_port != 0 while don't need extra proxy in remote,
   * set the expected proxy port for remote_port
   *
   * local_port: if =0, mean the
   * service is accessable in remote directly through MQTT, Otherwise, it mean
   * it need the remote to proxy the packet to local port
   */
  ServiceNode(const std::string &identifier, const int remote_port = 0,
              const int local_port = 0);

  // For register video stream
  ServiceNode(const std::string &identifier, const std::string &path);
  ~ServiceNode();

private:
  std::string SendUdp(const std::string &msg) const;
  // Get the ip for this service that the user used to access
  std::string RetreiveServiceIP() const;
  /**
   * If this service is a control service, it get a dynamic proxy port from BAT
   *
   * Otherwise, it return the BAT MQTT broker port
   */
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
