#pragma once

#include "include/common.h"

namespace camera {

class TrafiOneConfigHandler : public ConfigHandler {
public:
  TrafiOneConfigHandler();
  ~TrafiOneConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace camera
