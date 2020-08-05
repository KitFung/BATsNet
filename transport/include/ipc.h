#pragma once

#include <memory>
#include <mutex>

#include "cpp-ipc/include/ipc.h"

#include "transport/include/transport.h"

namespace transport {

template <typename T, typename std::enable_if<std::is_pod<T>::value, void>::type
                          * = nullptr>
bool ParseRecvData(const ipc::buff_t &buf_data, T *data) {
  *data = reinterpret_cast<const T *>(buf_data.data());
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
  IPC(const std::string channel) : channel_(channel) {}

  bool Receive(T *data, uint32_t timeout = 10000) override {

    std::call_once(receiver_flag_, [&]() {
      receiver_.reset(new ipc::channel(channel_.c_str(), ipc::receiver));
    });
    auto msg = receiver_->recv(timeout);
    ParseRecvData(msg, data);
    return true;
  }

protected:
  bool SendData(const char *data, const int32_t len) override {
    std::call_once(sender_flag_, [&]() {
      sender_.reset(new ipc::channel(channel_.c_str(), ipc::sender));
    });
    sender_->send(data, len);
    return true;
  }

private:
  std::string channel_;
  std::once_flag sender_flag_, receiver_flag_;
  std::shared_ptr<ipc::channel> sender_;
  std::shared_ptr<ipc::channel> receiver_;
};

} // namespace transport