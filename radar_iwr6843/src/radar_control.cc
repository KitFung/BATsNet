#include "include/radar_control.h"

#include <fstream>
#include <iostream>

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

Status RadarControl::SetManualCfg(ServerContext *context,
                                  const LadarCfg *request, DeviceConf *reply) {
  // Write the request to the cfg file
  const auto possible_mode = conf_.possible_mode();
  bool allow_manual = possible_mode.find(kUserCfgKey) != possible_mode.end();
  reply->CopyFrom(device_conf_);
  if (!allow_manual) {
    return Status::CANCELLED;
  }
  auto cfg_fpath = possible_mode.at(kUserCfgKey);
  std::ofstream f(cfg_fpath);
  f << request->data() << std::endl;
  f.close();

  RestartDevice();
  return Status::OK;
}

} // namespace radar
