#pragma once

#include "include/common.h"

namespace lidar {

class VelodyneConfigHandler : public ConfigHandler {
public:
  VelodyneConfigHandler();
  ~VelodyneConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace lidar
