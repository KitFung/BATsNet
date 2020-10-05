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

#include "include/common.h"

namespace camera {

void Deamon(int sig);

class CameraControl {
public:
  CameraControl(const ControllerConf &conf,
                const ControllerMutableState &state);
  bool HandleHewConf(const ControllerMutableState &new_state);

private:
  void BuildHandler(const CameraModel &model);
  void StartCamera();
  void StopCamera();

  ControllerConf conf_;
  ControllerMutableState state_;
  std::unique_ptr<ConfigHandler> conf_handler_;

  bool running_ = true;
  std::mutex mtx_;
  pid_t pid_ = 0;
  std::thread camera_deamon_;

  friend void Deamon(int);
};

} // namespace camera