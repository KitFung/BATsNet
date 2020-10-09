#ifndef VELODYNE_POINTCLOUD_DATACONTAINERBASE_H
#define VELODYNE_POINTCLOUD_DATACONTAINERBASE_H
// Copyright (C) 2012, 2019 Austin Robot Technology, Jack O'Quin, Joshua
// Whitley, Sebastian Pütz All rights reserved.
//
// Software License Agreement (BSD License 2.0)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//  * Neither the name of {copyright_holder} nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <Eigen/Dense>

#include <algorithm>
#include <cstdarg>
#include <memory>
#include <string>

#include "proto/velodyne.pb.h"

namespace velodyne_rawdata {
class DataContainerBase {
public:
  DataContainerBase(const double max_range, const double min_range,
                    const unsigned int num_lasers,
                    const unsigned int scans_per_block)
      : config_(max_range, min_range, num_lasers, scans_per_block) {}

  struct Config {
    double max_range; ///< maximum range to publish
    double min_range; ///< minimum range to publish
    unsigned int num_lasers;
    unsigned int scans_per_block;

    Config(double max_range, double min_range, const unsigned int num_lasers,
           unsigned int scans_per_block)
        : max_range(max_range), min_range(min_range), num_lasers(num_lasers),
          scans_per_block(scans_per_block) {}
  };

  virtual void setup(const velodyne::VelodyneScan &scan_msg) {
    cloud.set_stamp(scan_msg.stamp());
    cloud.clear_point();
    cloud.mutable_point()->Reserve(
        scan_msg.packets_size() * config_.scans_per_block * config_.num_lasers);
  }

  virtual void addPoint(float x, float y, float z, const uint16_t ring,
                        const uint16_t azimuth, const float distance,
                        const float intensity, const float time) = 0;
  virtual void newLine() = 0;

  const velodyne::PointCloud &finishCloud() { return cloud; }

  void configure(const double max_range, const double min_range) {
    config_.max_range = max_range;
    config_.min_range = min_range;
  }

  velodyne::PointCloud cloud;

  inline bool pointInRange(float range) {
    return (range >= config_.min_range && range <= config_.max_range);
  }

protected:
  Config config_;
  std::string sensor_frame;
};
} /* namespace velodyne_rawdata */
#endif // VELODYNE_POINTCLOUD_DATACONTAINERBASE_H
