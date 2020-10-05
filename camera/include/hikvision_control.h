#pragma once

#include "include/common.h"

namespace camera {

class HikvisionConfigHandler : public ConfigHandler {
public:
  HikvisionConfigHandler() {}
  ~HikvisionConfigHandler() override;
  bool UpdateConfig(const ControllerMutableState &state) override;
};

} // namespace camera