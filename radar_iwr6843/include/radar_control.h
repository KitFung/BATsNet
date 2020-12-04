#pragma once

#include "device/include/device_control.hpp"

#include "proto/radar.grpc.pb.h"

namespace radar {
constexpr auto kUserCfgKey = "USER_DEFINE";

using BaseControl =
    device::BaseControl<DeviceConf, ControllerConf, ControllerMutableState,
                        SetStateResponse, radar::Controller::Service>;

class RadarControl final : public BaseControl {
public:
  RadarControl(const DeviceConf &device_conf);
  Status SetManualCfg(ServerContext *context, const LadarCfg *request,
                      DeviceConf *reply) override;

protected:
  void BuildHandler() override;
};

} // namespace radar
