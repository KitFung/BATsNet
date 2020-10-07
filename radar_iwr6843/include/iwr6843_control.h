#pragma once

#include "include/common.h"

namespace radar {

class IWR68943ConfigHandler : public ConfigHandler {
public:
  IWR68943ConfigHandler();
  ~IWR68943ConfigHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace radar
