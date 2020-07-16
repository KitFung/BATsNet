#include "topic_saver.h"

namespace topic_saver {
TopicSaver::TopicSaver(const char *host, const int port) {
  SetupConnection(host, port);
}
void TopicSaver::SetupConnection(const char *host, const int port) {
  mosq_ = mosquitto_new(NULL, true, this);
  if (mosq_ == nullptr) {
    std::cerr << "Error while creating mosq instance" << std::endl;
    std::cerr << errno << " " << strerror(errno) << std::endl;
    exit(1);
  }
  mosquitto_connect_callback_set(mosq_, connect_callback);
  mosquitto_message_callback_set(mosq_, &TopicSaver::HandleTopicData_cb);

  int rc = mosquitto_connect(mosq_, host, port, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
    std::cerr << "Error while connection to broker " << host << " " << port
              << std::endl;
    std::cerr << errno << " " << strerror(errno) << std::endl;
    exit(1);
  }
}

void TopicSaver::RunUntilEnd() {
  while (true) {
    if (mosquitto_loop(mosq_, -1, 1) != MOSQ_ERR_SUCCESS) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      mosquitto_reconnect(mosq_);
    }
  }
}

AllTopicSaver::AllTopicSaver(const char *host, const int port,
                             const char *topic, const char *save_folder)
    : TopicSaver(host, port) {
  SubscribeToTopic(topic);
  for (int i = 0; i < kWriter; ++i) {
    writers_[i] = std::make_shared<BinWriter>(save_folder);
  }
}

void AllTopicSaver::HandleTopicData(const char *topic, const char *data,
                                    const int len) {
  std::string topic_str = std::string(topic);

  if (buffers_.find(topic_str) == buffers_.end()) {
    buffers_[topic_str] = std::make_shared<DataBuffer>(topic_str);
  }
  auto buf = buffers_[topic_str];
  buf->Append(data, len);
  total_len += len;

  // 100 k
  if (buf->ByteSize() > (1 << 10) * 100 || buf->message_len > 1000) {
    total_len -= buf->ByteSize();
    writers_[0]->AddTask(buf);
    buffers_.erase(topic_str);
  } else if (total_len > (1 << 20) * 20) {
    // > 20 MB
    size_t largest_size = 0;
    std::vector<std::string> keys;
    for (auto itr = buffers_.begin(); itr != buffers_.end(); itr++) {
      largest_size = std::max(largest_size, itr->second->ByteSize());
      keys.emplace_back(itr->first);
    }
    for (auto key : keys) {
      auto buf = buffers_[topic_str];
      if (buf->ByteSize() == largest_size) {
        total_len -= buf->ByteSize();
        writers_[0]->AddTask(buf);
        buffers_.erase(topic_str);
      }
    }
  }
}

} // namespace topic_saver