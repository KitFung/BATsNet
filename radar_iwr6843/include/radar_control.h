#pragma once

#include <memory>
#include <mutex>

#include "device/include/device_control.hpp"
#include "include/common.h"

#include "proto_gen/radar.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

namespace radar {

using BaseControl =
    device::BaseControl<DeviceConf, ControllerConf, ControllerMutableState,
                        SetStateResponse, radar::Controller::Service>;

class RadarControl final : public BaseControl {
public:
  RadarControl(const DeviceConf &device_conf);

protected:
  void BuildHandler() override;
};

} // namespace radar
