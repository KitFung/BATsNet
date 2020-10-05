#pragma once

#include "proto_gen/camera.pb.h"

namespace camera {

class ConfigHandler {
public:
  ConfigHandler() {}
  virtual ~ConfigHandler() {}
  virtual bool UpdateConfig(const ControllerMutableState &state) = 0;
};
} // namespace camera