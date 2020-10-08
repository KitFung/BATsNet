#pragma once

#include "device/include/config_handler.hpp"
#include "proto_gen/camera.pb.h"

namespace camera {

using ConfigHandler =
    common::ConfigHandler<ControllerMutableState, SetStateResponse>;
} // namespace camera