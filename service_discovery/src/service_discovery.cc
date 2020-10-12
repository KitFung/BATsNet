#include "include/service_discovery.h"

#include <boost/algorithm/string.hpp>

#include <vector>

namespace service_discovery {

std::string GetServicePath(const std::string &identifier) {
  auto etcd = std::make_shared<etcd::Client>(ketcd_src);
  auto res = etcd->get(identifier).get();
  if (res.error_code() != 0) {
    std::cerr << "Error for key: [" << identifier << "] " << res.error_code()
              << " " << res.error_message() << std::endl;
    return "";
  }
  return res.value().as_string();
}

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
