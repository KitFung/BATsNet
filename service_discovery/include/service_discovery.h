#pragma once

#include <memory>
#include <string>

#include "etcd/Client.hpp"

#include "include/common.h"

namespace service_discovery {

class ServiceHelper {
public:
  ServiceHelper();
  bool GetAddress(const std::string &identifier, std::string *address,
                  int *port);
  bool CheckAlive(const std::string &identifier);

private:
  void InitClient();
  std::shared_ptr<etcd::Client> etcd_;
};

} // namespace service_discovery
