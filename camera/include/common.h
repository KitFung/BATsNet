#pragma once

#include "device/include/config_handler.hpp"
#include "proto/camera.pb.h"

namespace camera {

using ConfigHandler =
    common::ConfigHandler<ControllerMutableState, SetStateResponse>;
} // namespace camera
