#pragma once

#include <iostream>
#include <memory>
#include <mutex>

#include "cpp-ipc/include/ipc.h"

#include "transport/include/transport.h"

// The broadcast function in cpp-ipc library have bug, cannot work properly
// after the queue is full. So only use it unicast now
using ipctype = ipc::chan<
    ipc::wr<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>>;

// using ipctype = ipc::chan<
//     ipc::wr<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast>>;

namespace transport {

template <typename T, typename std::enable_if<std::is_pod<T>::value, void>::type
                          * = nullptr>
bool ParseRecvData(const ipc::buff_t &buf_data, T *data) {
  memcpy(data, buf_data.data(), sizeof(T));
  return true;
}

template <typename T,
          typename std::enable_if<!std::is_pod<T>::value &&
                                      std::is_member_function_pointer<decltype(
                                          &T::SerializeAsString)>::value,
                                  void>::type * = nullptr>
bool ParseRecvData(const ipc::buff_t &buf_data, T *data) {
  data->ParseFromString(std::string(
      reinterpret_cast<const char *>(buf_data.data()), buf_data.size()));
  return true;
}

inline bool ParseRecvData(const ipc::buff_t &buf_data, std::string *data) {
  *data = std::string(reinterpret_cast<const char *>(buf_data.data()),
                      buf_data.size());
  return true;
}

template <typename T> class IPC : public Transport<T> {
public:
  IPC(const std::string channel) {
    channel_ = channel;
    for (auto &w : channel_) {
      if (w == '/') {
        w = '-';
      }
    }
  }

  bool Receive(T *data, uint32_t timeout = 10000) override {
    std::call_once(sender_flag_, [&]() {
      receiver_.reset(new ipctype(channel_.c_str(), ipc::sender));
    });
    auto msg = receiver_->recv(timeout);
    if (msg.empty()) {
      return false;
    }
    ParseRecvData(msg, data);
    return true;
  }

protected:
  bool SendData(const char *data, const int32_t len) override {
    std::call_once(sender_flag_, [&]() {
      sender_.reset(new ipctype(channel_.c_str(), ipc::sender));
    });
    return sender_->send(data, len);
  }

private:
  std::string channel_;
  std::once_flag sender_flag_, receiver_flag_;
  std::shared_ptr<ipctype> sender_;
  std::shared_ptr<ipctype> receiver_;
};

} // namespace transport