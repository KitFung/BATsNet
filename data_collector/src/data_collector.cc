#include "data_collector.h"

namespace data_collecter {
DataCollector::DataCollector(const char *host, int port,
                             DataCollectParams *params) {
  mosqpp::lib_init();
  if (params != nullptr) {
    params_ = *params;
  }
  host_ = std::string(host);
  port_ = port;

  connect_async(host, port, params_.keep_alive);
  loop_start();
}

DataCollector::~DataCollector() {
  loop_stop();
  mosqpp::lib_cleanup();
}

bool DataCollector::SendData(const char *topic, const char *data) {
  int ret = publish(NULL, topic, strlen(data), data, params_.qos, false);
  return (ret == MOSQ_ERR_SUCCESS);
}

bool DataCollector::SendData(const char *topic, const char *data,
                             const int len) {
  int ret = publish(NULL, topic, len, data, params_.qos, false);
  return (ret == MOSQ_ERR_SUCCESS);
}
} // namespace data_collecter