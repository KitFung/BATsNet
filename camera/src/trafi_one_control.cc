#include "include/trafi_one_control.h"

namespace camera {
    
TrafiOneConfigHandler::TrafiOneConfigHandler() {}
TrafiOneConfigHandler::~TrafiOneConfigHandler() {}
bool TrafiOneConfigHandler::UpdateConfig(const ControllerMutableState &state) {
  return true;
}

} // namespace camera