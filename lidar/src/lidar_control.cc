#include "include/lidar_control.h"

#include "include/livox_control.h"
#include "include/rslidar_control.h"
#include "include/velodyne_control.h"

namespace lidar {
LidarControl::LidarControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void LidarControl::BuildHandler() {
  auto model = conf_.model();
  switch (model) {
  case LidarModel::VELODYNE:
    ConfHandler().reset(new VelodyneConfigHandler());
    break;
  case LidarModel::RSLIDAR:
    ConfHandler().reset(new RslidarConfigHandler());
    break;
  case LidarModel::LIVOX:
    ConfHandler().reset(new LivoxConfigHandler());
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }
}
} // namespace lidar
