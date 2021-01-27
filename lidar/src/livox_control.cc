#include "include/livox_control.h"

namespace lidar {

LivoxConfigHandler::LivoxConfigHandler() {}
LivoxConfigHandler::~LivoxConfigHandler() {}
SetStateResponse
LivoxConfigHandler::UpdateConfig(const ControllerMutableState &old_state,
                                   const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace lidar
