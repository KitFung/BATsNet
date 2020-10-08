#include "include/rslidar_control.h"

namespace lidar {

RslidarConfigHandler::RslidarConfigHandler() {}
RslidarConfigHandler::~RslidarConfigHandler() {}
SetStateResponse
RslidarConfigHandler::UpdateConfig(const ControllerMutableState &old_state,
                                   const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace lidar
