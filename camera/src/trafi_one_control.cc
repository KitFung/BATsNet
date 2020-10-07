#include "include/trafi_one_control.h"

namespace camera {

TrafiOneConfigHandler::TrafiOneConfigHandler() {}
TrafiOneConfigHandler::~TrafiOneConfigHandler() {}
SetStateResponse
TrafiOneConfigHandler::UpdateConfig(const ControllerMutableState &old_state,
                                    const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace camera
