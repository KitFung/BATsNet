#pragma once

#include "include/common.h"

namespace lidar {

class RslidarConfigHandler : public ConfigHandler {
public:
  RslidarConfigHandler();
  ~RslidarConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace lidar
