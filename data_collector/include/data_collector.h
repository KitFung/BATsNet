#include <cstring>

#include <string>

#include <mosquitto.h>
#include <mosquittopp.h>

namespace data_collecter {
struct DataCollectParams {
  bool auto_adjust;
  int min_freq = 1; // hz
  int keep_alive = 60;
  double adjust_step = 0.2;
  int qos = 1; // 0 = try once, 1 = retry 1 max, 2 retry until done
};

class DataCollector : public mosqpp::mosquittopp {
public:
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
};
} // namespace data_collecter