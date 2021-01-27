#pragma once

#include <cstring>

#include <memory>
#include <string>
#include <unordered_map>

#include <mosquitto.h>
#include <mosquittopp.h>

#include "data_collector/include/data_io.h"

namespace data_collector {
enum class CollectMode { REAL_TIME_UPLOAD = 1, LOCAL_SAVE = 2 };

struct DataCollectParams {
  bool auto_adjust;
  int min_freq = 1; // hz
  int keep_alive = 60;
  double adjust_step = 0.2;
  int qos = 1; // 0 = try once, 1 = retry 1 max, 2 retry until done
  CollectMode mode = CollectMode::REAL_TIME_UPLOAD;
};

class DataCollector : public mosqpp::mosquittopp {
public:
  DataCollector();
  DataCollector(const char *host, const int port,
                DataCollectParams *params = nullptr);
  ~DataCollector();

  // void on_connect(int rc);
  // void on_disconnect(int rc);
  // void on_publish(int mid);

  bool SendData(const char *topic, const char *data);
  bool SendData(const char *topic, const char *data, const int len);

private:
  std::string host_;
  int port_;
  DataCollectParams params_;

  std::unordered_map<std::string, std::shared_ptr<BufWriter>> writer_;
  std::unordered_map<std::string, int> buf_cnt_;
};
} // namespace data_collector
