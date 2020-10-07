#pragma once

#include "proto_gen/camera.pb.h"

namespace camera {

class ConfigHandler {
public:
  ConfigHandler() {}
  virtual ~ConfigHandler() {}
  virtual SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) = 0;
};
} // namespace camera
