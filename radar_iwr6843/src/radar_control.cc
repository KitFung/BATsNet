#include "include/radar_control.h"

#include <fstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "include/iwr6843_control.h"

namespace radar {
RadarControl::RadarControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void RadarControl::BuildHandler() {
  auto model = conf_.model();
  std::cout << "BuildHandler called" << std::endl;
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
