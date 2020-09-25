#pragma once

#include <string>
#include <type_traits>

namespace transport {

template <typename T> class Transport;

template <typename T, typename std::enable_if<std::is_pod<T>::value, void>::type
                          * = nullptr>
bool SendDataFn(Transport<T> *obj, const T &data) {
  return obj->SendData(reinterpret_cast<const char *>(&data), sizeof(T));
}

template <typename T,
          typename std::enable_if<!std::is_pod<T>::value &&
                                      std::is_member_function_pointer<decltype(
                                          &T::SerializeAsString)>::value,
                                  void>::type * = nullptr>
bool SendDataFn(Transport<T> *obj, const T &data) {
  const auto value = data.SerializeAsString();
  return obj->SendData(value.data(), value.size());
}

template <typename T,
          typename std::enable_if<std::is_base_of<T, std::string>::value,
                                  void>::type * = nullptr>
bool SendDataFn(Transport<T> *obj, const T &data) {
  return obj->SendData(reinterpret_cast<const char *>(data.data()),
                       data.size());
}

template <typename T> class Transport {
public:
  Transport() {}
  virtual ~Transport() {}

  bool Send(const T &data) { return SendDataFn(this, data); }

  // Let the receive to customize in each sub class.
  virtual bool Receive(T *data, uint32_t timeout = 10000) = 0;
  virtual bool SendData(const char *data, const int32_t len) = 0;

protected:
  std::string channel_;
};
} // namespace transport