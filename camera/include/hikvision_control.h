#pragma once

#include "include/common.h"

namespace camera {

class HikvisionConfigHandler : public ConfigHandler {
public:
  HikvisionConfigHandler() {}
  ~HikvisionConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace camera
