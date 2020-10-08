#pragma once

#include "device/include/config_handler.hpp"
#include "proto_gen/lidar.pb.h"

namespace lidar {

using ConfigHandler =
    common::ConfigHandler<ControllerMutableState, SetStateResponse>;
} // namespace lidar
