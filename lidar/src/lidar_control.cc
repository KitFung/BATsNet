#include "include/lidar_control.h"

#include "include/rslidar_control.h"
#include "include/velodyne_control.h"

namespace lidar {
LidarControl::LidarControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void LidarControl::BuildHandler() {
  auto model = conf_.model();
  std::cout << "BuildHandler called" << std::endl;
  switch (model) {
  case LidarModel::VELODYNE:
    // ConfHandler().reset(new IWR68943ConfigHandler());
    break;
  case LidarModel::RSLIDAR:
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }
}
} // namespace lidar
