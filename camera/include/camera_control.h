#pragma once

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "device/include/device_deamon.hpp"
#include "include/common.h"

#include "proto_gen/camera.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

namespace camera {

class CameraControl;
using CameraDeamon = device::DeviceDeamon<CameraControl>;

class CameraControl final : public camera::Controller::Service {
public:
  CameraControl(const DeviceConf &device_conf);
  // API
  Status GetConf(ServerContext *context, const common::Empty *request,
                 ControllerConf *reply) override;
  Status GetState(ServerContext *context, const common::Empty *request,
                  ControllerMutableState *reply) override;
  Status SetState(ServerContext *context, const ControllerMutableState *request,
                  SetStateResponse *reply) override;

private:
  void BuildHandler(const CameraModel &model);
  bool HandleHewConf(const ControllerMutableState &new_state,
                     SetStateResponse *reply);
  void WriteNewDeviceConf();

  ControllerConf conf_;
  ControllerMutableState state_;
  DeviceConf device_conf_;
  std::unique_ptr<ConfigHandler> conf_handler_;

  bool running_ = true;
  std::mutex state_mtx_;
  pid_t pid_ = 0;
  std::unique_ptr<CameraDeamon> deamon_;
};

} // namespace camera
