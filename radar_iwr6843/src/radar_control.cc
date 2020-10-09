#include "include/radar_control.h"

#include "include/iwr6843_control.h"

namespace radar {
RadarControl::RadarControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void RadarControl::BuildHandler() {
  auto model = conf_.model();
  switch (model) {
  case Model::IWR6843:
    ConfHandler().reset(new IWR68943ConfigHandler());
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }
}
} // namespace radar
