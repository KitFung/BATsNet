#include "livox_driver.h"

namespace livox {

namespace {
double NowSec() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() /
         1000.0;
}
} // namespace
livoxDriver::livoxDriver(Conf conf) {
  std::vector<std::string> cmdline_broadcast_code = {
      conf.device_broadcast_code()};
  packet_per_publish_ =
      GetPacketNumPerSec(kDeviceTypeLidarHorizon, kExtendCartesian) * 100.0 /
      1000;
  read_lidar_ = &LdsLidar::GetInstance();
  read_lidar_->pointcloud_handle_ = [this](auto begin, auto end) {
    this->HandlePointCloud(begin, end);
  };
  int ret = read_lidar_->InitLdsLidar(cmdline_broadcast_code);
}

livoxDriver::~livoxDriver() { read_lidar_->DeInitLdsLidar(); }

void livoxDriver::HandlePointCloud(LivoxPointXyzrtl *begin,
                                   LivoxPointXyzrtl *end) {
  std::lock_guard<std::mutex> lock(mtx_);
  // A brute check for avoid the pointcloud is not using
  if (accumulated_packet_ > packet_per_publish_ * 50) {
    accumulated_packet_ = 0;
    cloud_.Clear();
  }
  if (accumulated_packet_ == 0) {
    cloud_.set_stamp(NowSec());
  }
  for (LivoxPointXyzrtl *itr = begin; itr != end; ++itr) {
    if (itr->x == 0 && itr->y == 0 && itr->z == 0) {
      continue;
    }
    auto pt = cloud_.add_point();
    pt->set_x(itr->x);
    pt->set_y(itr->y);
    pt->set_z(itr->z);
    pt->set_reflectivity(itr->reflectivity);
    pt->set_tag(itr->tag);
    pt->set_line(itr->line);
  }
  accumulated_packet_++;
}
bool livoxDriver::poll(PointCloud *cloud) {
  if (accumulated_packet_ >= packet_per_publish_) {
    std::lock_guard<std::mutex> lock(mtx_);
    cloud->CopyFrom(cloud_);
    cloud_.Clear();
    accumulated_packet_ = 0;
  }
  return true;
}

} // namespace livox
