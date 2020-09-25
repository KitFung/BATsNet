#include "include/service_discovery.h"

#include <boost/algorithm/string.hpp>

#include <vector>

namespace service_discovery {
ServiceHelper::ServiceHelper() { InitClient(); }

bool ServiceHelper::GetAddress(const std::string &identifier,
                               std::string *address, int *port) {
  auto res = etcd_->get(identifier).get();
  if (res.error_code() != 0) {
    return false;
  }
  const auto &val = res.value().as_string();

  std::vector<std::string> strs;
  boost::split(strs, val, boost::is_any_of(":"));

  *address = strs[0];
  *port = std::stoi(strs[1]);
  return true;
}

bool ServiceHelper::CheckAlive(const std::string &identifier) {
  return etcd_->get(identifier).get().error_code() == 0;
}

void ServiceHelper::InitClient() {
  etcd_ = std::make_shared<etcd::Client>(ketcd_src);
}

} // namespace service_discovery