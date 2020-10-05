#pragma once

#include "include/common.h"

namespace camera {

class TrafiOneConfigHandler : public ConfigHandler {
public:
  TrafiOneConfigHandler();
  ~TrafiOneConfigHandler() override;
  bool UpdateConfig(const ControllerMutableState &state) override;
};

} // namespace camera