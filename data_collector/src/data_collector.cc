#include "data_collector.h"

#include <iostream>
namespace data_collector {
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
  loop_stop(true);
  mosqpp::lib_cleanup();
}

bool DataCollector::SendData(const char *topic, const char *data) {
  return SendData(topic, data, strlen(data));
}

bool DataCollector::SendData(const char *topic, const char *data,
                             const int len) {
  if (params_.mode == CollectMode::REAL_TIME_UPLOAD) {
    int ret = publish(NULL, topic, len, data, params_.qos, false);
    return (ret == MOSQ_ERR_SUCCESS);
  } else {
    if (buf_cnt_[topic] % 100 == 0) {
      writer_[topic].reset(new BufWriter(topic));
      buf_cnt_[topic] = 0;
      std::cout << "Reset BufWriter" << std::endl;
    }
    writer_[topic]->Write(data, len);
    buf_cnt_[topic] += 1;
    return true;
  }
}
} // namespace data_collector