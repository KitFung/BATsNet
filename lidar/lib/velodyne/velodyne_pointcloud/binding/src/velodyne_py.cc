#include <iostream>
#include <memory>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "lidar/lib/velodyne/velodyne_pointcloud/include/velodyne_pointcloud/transform.h"
#include "transport/include/block_channel.h"

namespace py = pybind11;

constexpr int kMaxPoint = 20000;

class VelodyneCloudReader {
public:
  VelodyneCloudReader(const std::string &name, const std::string &conf_str) {
    velodyne::VelodynePointCloudConf conf;
    conf.ParseFromString(conf_str);
    channel_ =
        std::make_unique<transport::BlockChannel<velodyne::VelodyneScan>>(name);
    transform_ = std::make_unique<velodyne_pointcloud::Transform>(conf);
    memset(point_buf_, 0, sizeof(point_buf_));
  }

  int GetClouds(int timeout_ms, float **buf) {
    velodyne::VelodyneScan scan;
    *buf = &point_buf_[0][0];
    if (channel_->Receive(&scan, timeout_ms)) {
      transform_->processScan(scan, false);
      auto cloud = transform_->cloud();
      int n = cloud.point_size();
      std::cout << "n: " << n << std::endl;
      if (n > kMaxPoint) {
        n = kMaxPoint;
      }
      int i = 0;
      for (const auto &point : cloud.point()) {
        point_buf_[i][0] = point.x();
        point_buf_[i][1] = point.y();
        point_buf_[i][2] = point.z();
        point_buf_[i][3] = point.intensity();
        ++i;
      }
      return n;
    }
    return 0;
  }

private:
  std::unique_ptr<transport::BlockChannel<velodyne::VelodyneScan>> channel_;
  std::unique_ptr<velodyne_pointcloud::Transform> transform_;
  float point_buf_[kMaxPoint][4];
};

PYBIND11_MODULE(velodyne_py, m) {
  m.doc() = "The py-binding of the velodyne cloud reader library";

  py::class_<VelodyneCloudReader>(m, "VelodyneCloudReader")
      .def(py::init<const std::string &, const std::string &>())
      .def(
          "recv",
          [&](VelodyneCloudReader &reader, uint32_t time_out) {
            float *buf;
            int npoint = reader.GetClouds(time_out, &buf);
            return py::array_t<float>({npoint, 4}, buf);
          },
          py::arg("time_out") = 10000);
}
