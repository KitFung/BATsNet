#include "include/velodyne_control.h"

namespace lidar {

VelodyneConfigHandler::VelodyneConfigHandler() {}
VelodyneConfigHandler::~VelodyneConfigHandler() {}
SetStateResponse
VelodyneConfigHandler::UpdateConfig(const ControllerMutableState &old_state,
                                    const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace lidar
