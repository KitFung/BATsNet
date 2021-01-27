#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

#include "lds_lidar.h"
#include "proto/livox.pb.h"

namespace livox {
class livoxDriver {
public:
  livoxDriver(Conf conf);

  ~livoxDriver();

  bool poll(PointCloud *cloud);

private:
  void HandlePointCloud(LivoxPointXyzrtl *begin, LivoxPointXyzrtl *end);
  LdsLidar *read_lidar_;

  std::mutex mtx_;
  int packet_per_publish_ = 250;
  std::atomic<int> accumulated_packet_ = {0};
  PointCloud cloud_;
};

} // namespace livox
