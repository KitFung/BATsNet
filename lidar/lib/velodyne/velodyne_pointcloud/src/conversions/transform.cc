/*
 *  Copyright (C) 2009, 2010 Austin Robot Technology, Jack O'Quin
 *  Copyright (C) 2011 Jesse Vera
 *  Copyright (C) 2012 Austin Robot Technology, Jack O'Quin
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** @file

    This class transforms raw Velodyne 3D LIDAR packets to PointCloud2
    in the /map frame of reference.

    @author Jack O'Quin
    @author Jesse Vera
    @author Sebastian Pütz

*/

#include "velodyne_pointcloud/transform.h"

#include <optional>

#include <velodyne_pointcloud/pointcloudXYZIRT.h>

namespace velodyne_pointcloud {
/** @brief Constructor. */
Transform::Transform(const velodyne::VelodynePointCloudConf &conf)
    : data_(new velodyne_rawdata::RawData) {
  boost::optional<velodyne_pointcloud::Calibration> calibration =
      data_->setup(conf);
  if (calibration) {
    std::cout << "Calibration file loaded." << std::endl;
    config_.num_lasers = static_cast<uint16_t>(calibration.get().num_lasers);
  } else {
    std::cerr << "Could not load calibration file!" << std::endl;
  }
  config_.max_range = conf.max_range();
  config_.min_range = conf.min_range();
  container_ptr = std::shared_ptr<PointcloudXYZIRT>(
      new PointcloudXYZIRT(config_.max_range, config_.min_range,
                           config_.num_lasers, data_->scansPerPacket()));

  // advertise output point cloud (before subscribing to input data)
  // output_ = node.advertise<sensor_msgs::PointCloud2>("velodyne_points", 10);
  data_->setParameters(config_.min_range, config_.max_range, 0, M_PI * 2);
  // data_->setParameters(config_.min_range, config_.max_range, 0, M_PI * 2);

  output_ = std::make_shared<transport::IPC<velodyne::PointCloud>>(
      conf.cloud_topic_name());
  velodyne_scan_ = std::make_shared<transport::IPC<velodyne::VelodyneScan>>(
      conf.scan_topic_name());
}
/** @brief Callback for raw scan messages.
 *
 *  @pre TF message filter has already waited until the transform to
 *       the configured @c frame_id can succeed.
 */
void Transform::processScan(const velodyne::VelodyneScan &scanMsg,
                            bool publish_cloud) {
  // if (output_.getNumSubscribers() == 0) // no one listening?
  //   return;                             // avoid much work

  // allocate a point cloud with same time and frame ID as raw data
  container_ptr->setup(scanMsg);

  // process each packet provided by the driver
  for (size_t i = 0; i < scanMsg.packets_size(); ++i) {
    // calculate individual transform for each packet to account for ego
    // during one rotation of the velodyne sensor
    data_->unpack(scanMsg.packets(i), *container_ptr, scanMsg.stamp());
  }
  // publish the accumulated cloud message
  if (publish_cloud) {
    output_->Send(container_ptr->finishCloud());
    // std::cout << "point size: " << container_ptr->finishCloud().point_size()
    //           << std::endl;
  }
}

void Transform::Start() {
  velodyne::VelodyneScan scan;
  while (true) {
    // std::cout << "wait scan" << std::endl;
    if (velodyne_scan_->Receive(&scan, 1000)) {
      // std::cout << "processScan" << std::endl;
      processScan(scan);
      scan.Clear();
    }
  }
}

} // namespace velodyne_pointcloud
