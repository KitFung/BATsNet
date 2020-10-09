#pragma once

#include "device/include/config_handler.hpp"
#include "proto/radar.pb.h"

namespace radar {

using ConfigHandler =
    common::ConfigHandler<ControllerMutableState, SetStateResponse>;
} // namespace radar
