#pragma once

#include <memory>
#include <string>

#include "etcd/Client.hpp"

#include "include/common.h"

namespace service_discovery {

std::string GetServicePath(const std::string &identifier);

/**
 * A helper class that accessing the service registry
 * and return the service information
 */
class ServiceHelper {
public:
  ServiceHelper();
  /**
   * identifier: The service name
   * address, port: The accessing path for the service
   * return true if the query success
   */
  bool GetAddress(const std::string &identifier, std::string *address,
                  int *port);
  /**
   * identifier: The service name
   * return true if the service is alive and the query success
   */
  bool CheckAlive(const std::string &identifier);

private:
  void InitClient();
  std::shared_ptr<etcd::Client> etcd_;
};

} // namespace service_discovery
