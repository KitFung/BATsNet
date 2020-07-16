#include <string.h>

#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

#include <mosquitto.h>

#include "bin_writer.h"

namespace topic_saver {

constexpr int kWriter = 4;

void connect_callback(struct mosquitto *mosq, void *obj, int result) {
  printf("connect callback, rc=%d\n", result);
}

class TopicSaver {
public:
  TopicSaver(const char *host, const int port);
  static void HandleTopicData_cb(struct mosquitto *mosq, void *obj,
                                 const struct mosquitto_message *message) {
    TopicSaver *saver = reinterpret_cast<TopicSaver *>(obj);
    saver->HandleTopicData(message->topic,
                           reinterpret_cast<const char *>(message->payload),
                           message->payloadlen);
  }

  virtual void HandleTopicData(const char *topic, const char *data,
                               const int len) = 0;
  void RunUntilEnd();

protected:
  void SubscribeToTopic(const char *topic_pattern) {
    mosquitto_subscribe(mosq_, NULL, topic_pattern, 1);
  }

  void SetupConnection(const char *host, const int port);

  mosquitto *mosq_ = nullptr;
  std::function<void(const mosquitto_message *)> cb_fn_;
};

// class SingleTopicSaver : public TopicSaver {
// public:
//   SingleTopicSaver(const char *topic, const char *save_folder);

// private:
//   std::shared_ptr<BinWriter> writer_;
// };

class AllTopicSaver : public TopicSaver {
public:
  AllTopicSaver(const char *host, const int port, const char *topic,
                const char *save_folder);
  void HandleTopicData(const char *topic, const char *data,
                       const int len) override;

private:
  std::unordered_map<std::string, std::shared_ptr<DataBuffer>> buffers_;
  std::array<std::shared_ptr<BinWriter>, kWriter> writers_;
  int total_len = 0;
};

} // namespace topic_saver