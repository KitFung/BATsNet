#pragma once

#include "device/include/device_control.hpp"

#include "proto/lidar.grpc.pb.h"
#include "proto/lidar.pb.h"

namespace lidar {

using BaseControl =
    device::BaseControl<DeviceConf, ControllerConf, ControllerMutableState,
                        SetStateResponse, lidar::Controller::Service>;

class LidarControl final : public BaseControl {
public:
  LidarControl(const DeviceConf &device_conf);

protected:
  void BuildHandler() override;
};

} // namespace lidar
