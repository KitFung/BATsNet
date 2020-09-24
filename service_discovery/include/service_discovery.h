#include <string>

#include "include/common.h"

namespace service_discovery {

class ServiceHelper {
public:
  bool GetAddress(const std::string &identifier, std::string *address,
                  int *port);
  bool CheckAlive();

private:

};

} // namespace service_discovery
