#pragma once

#include "device/include/device_control.hpp"

#include "proto_gen/camera.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

namespace camera {

using BaseControl =
    device::BaseControl<DeviceConf, ControllerConf, ControllerMutableState,
                        SetStateResponse, camera::Controller::Service>;

class CameraControl final : public BaseControl {
public:
  CameraControl(const DeviceConf &device_conf);

protected:
  void BuildHandler() override;
};

} // namespace camera
