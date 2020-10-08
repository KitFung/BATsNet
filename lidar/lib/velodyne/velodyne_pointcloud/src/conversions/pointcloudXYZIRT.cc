
#include <velodyne_pointcloud/pointcloudXYZIRT.h>

namespace velodyne_pointcloud {
PointcloudXYZIRT::PointcloudXYZIRT(const double max_range,
                                   const double min_range,
                                   const unsigned int num_lasers,
                                   const unsigned int scans_per_block)
    : DataContainerBase(max_range, min_range, num_lasers, scans_per_block) {}

void PointcloudXYZIRT::newLine() {}

void PointcloudXYZIRT::setup(const velodyne::VelodyneScan &scan_msg) {
  DataContainerBase::setup(scan_msg);
}

void PointcloudXYZIRT::addPoint(float x, float y, float z, const uint16_t ring,
                                const uint16_t /*azimuth*/,
                                const float distance, const float intensity,
                                const float time) {
  /** The laser values are not ordered, the organized structure
   * needs ordered neighbour points. The right order is defined
   * by the laser_ring value.
   */
  if (pointInRange(distance)) {
    // transformPoint(x, y, z);
    auto point = cloud.add_point();
    point->set_x(x);
    point->set_y(y);
    point->set_z(z);
    point->set_intensity(intensity);
    point->set_ring(ring);
    point->set_time(time);
  }
}
} // namespace velodyne_pointcloud