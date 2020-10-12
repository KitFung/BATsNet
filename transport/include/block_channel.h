#pragma once

#include "transport/include/transport.h"

#include <mosquitto.h>
#include <mosquittopp.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "service_discovery/include/service_discovery.h"

namespace transport {

template <typename T, typename std::enable_if<std::is_pod<T>::value, void>::type
                          * = nullptr>
bool ParseRecvData(const mosquitto_message *message, T *data) {
  memcpy(data, reinterpret_cast<const char *>(message->payload), sizeof(T));
  return true;
}

template <typename T,
          typename std::enable_if<!std::is_pod<T>::value &&
                                      std::is_member_function_pointer<decltype(
                                          &T::SerializeAsString)>::value,
                                  void>::type * = nullptr>
bool ParseRecvData(const mosquitto_message *message, T *data) {
  data->ParseFromArray(reinterpret_cast<const char *>(message->payload),
                       message->payloadlen);
  return true;
}

inline bool ParseRecvData(const mosquitto_message *message, std::string *data) {
  *data = std::string(reinterpret_cast<const char *>(message->payload),
                      message->payloadlen);
  return true;
}

template <typename T>
class BlockChannel : public Transport<T>, public mosqpp::mosquittopp {
public:
  BlockChannel(const std::string channel) {
    mosqpp::lib_init();
    channel_ = channel;
  }

  ~BlockChannel() override {
    loop_stop(true);
    mosqpp::lib_cleanup();
  }

  bool Receive(T *data, uint32_t timeout = 10000) override {
    if (read_actived_ && !helper_.CheckAlive(channel_)) {
      disconnect();
      connected_ = false;
      read_actived_ = false;
      std::cout << "[BlockChannel] Detect service down" << std::endl;
      return false;
    }

    if (!connected_ && !InitMos()) {
      return false;
    }
    if (!read_actived_ && !StartRead()) {
      return false;
    }

    {
      std::unique_lock<std::mutex> lock(q_mtx_);
      if (msg_queue_.empty()) {
        if (!q_cv_.wait_for(lock, std::chrono::milliseconds(timeout),
                            [this] { return !msg_queue_.empty(); })) {
          return false;
        }
      }
    }
    std::lock_guard<std::mutex> lock(q_mtx_);
    *data = msg_queue_.front();
    msg_queue_.pop();
    return true;
  }

private:
  bool SendData(const char *data, const int32_t len) override {
    if (!connected_) {
      InitMos();
    }
    int ret = publish(NULL, channel_.c_str(), len, data, 2, false);
    return (ret == MOSQ_ERR_SUCCESS);
  }

  bool InitMos() {
    // Step 1. Get Destination
    if (!helper_.GetAddress(channel_, &addr_, &port_)) {
      std::cerr << "[BlockChannel] Failed to found existing channel"
                << std::endl;
      return false;
    }
    std::cout << "[BlockChannel] MOS Connecting to " << addr_ << " " << port_
              << std::endl;
    connect(addr_.c_str(), port_);
    std::cout << "[BlockChannel] MOS Connected to " << addr_ << " " << port_
              << std::endl;
    loop_start();
    connected_ = true;

    return true;
  }

  bool StartRead() {
    read_actived_ = true;
    subscribe(NULL, channel_.c_str());
    return true;
  }

  void on_message(const mosquitto_message *message) override {
    std::lock_guard<std::mutex> lock(q_mtx_);
    T msg;
    // std::cout << "message->payloadlen: " << message->payloadlen << std::endl;
    ParseRecvData(message, &msg);
    msg_queue_.emplace(std::move(msg));
    if (msg_queue_.size() > queue_max_size_) {
      msg_queue_.pop();
    }
    q_cv_.notify_all();
  }

  std::string channel_;
  bool read_actived_ = false;
  bool connected_ = false;
  std::string addr_;
  int port_ = 0;
  service_discovery::ServiceHelper helper_;

  const int queue_max_size_ = 1;
  std::mutex q_mtx_;
  std::condition_variable q_cv_;
  std::queue<T> msg_queue_;
};

} // namespace transport
