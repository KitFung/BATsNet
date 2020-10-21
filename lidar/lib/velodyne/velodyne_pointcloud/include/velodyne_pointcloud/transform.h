// Copyright (C) 2009, 2010, 2011, 2012, 2019 Austin Robot Technology, Jack
// O'Quin, Jesse Vera, Joshua Whitley, Sebastian Pütz All rights reserved.
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

/** @file

    This class transforms raw Velodyne 3D LIDAR packets to PointCloud2
    in the /map frame of reference.

*/

#ifndef VELODYNE_POINTCLOUD_TRANSFORM_H
#define VELODYNE_POINTCLOUD_TRANSFORM_H

#include <memory>
#include <string>

#include <velodyne_pointcloud/pointcloudXYZIRT.h>
#include <velodyne_pointcloud/rawdata.h>

#include "proto/velodyne.pb.h"

#include "transport/include/ipc.h"

namespace velodyne_pointcloud {

class Transform {
public:
  Transform(const velodyne::VelodynePointCloudConf &conf);
  ~Transform() {}
  void processScan(const velodyne::VelodyneScan &scanMsg,
                   bool publish_cloud = true);
  void Start();
  inline const velodyne::PointCloud &cloud() {
    return container_ptr->finishCloud();
  }

private:
  std::shared_ptr<velodyne_rawdata::RawData> data_;
  std::shared_ptr<transport::IPC<velodyne::VelodyneScan>> velodyne_scan_;
  std::shared_ptr<transport::IPC<velodyne::PointCloud>> output_;

  /// configuration parameters
  typedef struct {
    double max_range;    ///< maximum range to publish
    double min_range;    ///< minimum range to publish
    uint16_t num_lasers; ///< number of lasers
  } Config;
  Config config_;

  std::shared_ptr<velodyne_rawdata::DataContainerBase> container_ptr;
};
} // namespace velodyne_pointcloud

#endif // VELODYNE_POINTCLOUD_TRANSFORM_H
