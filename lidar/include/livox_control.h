#pragma once

#include "include/common.h"

namespace lidar {

class LivoxConfigHandler : public ConfigHandler {
public:
  LivoxConfigHandler();
  ~LivoxConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace lidar
