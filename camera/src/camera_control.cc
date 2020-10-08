#include "include/camera_control.h"

#include "include/hikvision_control.h"
#include "include/trafi_one_control.h"

namespace camera {

CameraControl::CameraControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void CameraControl::BuildHandler() {
  auto model = conf_.model();
  switch (model) {
    //   case CameraModel::HIKVISION_21XX:
    //     conf_handler_ = std::make_unique<HikvisionConfigHandler>();
    //     break;
  case CameraModel::TRAFI_ONE_195:
    ConfHandler().reset(new TrafiOneConfigHandler());
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }
}

} // namespace camera
