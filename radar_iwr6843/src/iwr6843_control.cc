#include "include/iwr6843_control.h"

namespace radar {

IWR68943ConfigHandler::IWR68943ConfigHandler() {}
IWR68943ConfigHandler::~IWR68943ConfigHandler() {}
SetStateResponse
IWR68943ConfigHandler::UpdateConfig(const ControllerMutableState &old_state,
                                    const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace radar
