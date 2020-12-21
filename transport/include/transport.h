#pragma once

#include <string>
#include <type_traits>

namespace transport {

template <typename T> class Transport;

/**
 * Let obj to send data with POD datatype
 */
template <typename T, typename std::enable_if<std::is_pod<T>::value, void>::type
                          * = nullptr>
bool SendDataFn(Transport<T> *obj, const T &data) {
  return obj->SendData(reinterpret_cast<const char *>(&data), sizeof(T));
}

/**
 * Let obj to send data with protobuf datatype
 */
template <typename T,
          typename std::enable_if<!std::is_pod<T>::value &&
                                      std::is_member_function_pointer<decltype(
                                          &T::SerializeAsString)>::value,
                                  void>::type * = nullptr>
bool SendDataFn(Transport<T> *obj, const T &data) {
  const auto value = data.SerializeAsString();
  return obj->SendData(value.data(), value.size());
}

/**
 * Let obj to send string data
 */
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

  /***
   * Accept sending data with all datatype
   * 
   * return true if send success. False otherwise
   */
  bool Send(const T &data) { return SendDataFn(this, data); }
  /**
   * Let the subclass to implement how to receive data
   * unit of timeout is ms
   * 
   * return true if receive success. False otherwise
   */
  virtual bool Receive(T *data, uint32_t timeout = 10000) = 0;

  /**
   *  This function was not suppose to be called by user explicited.
   *  The only reason for put it in public, is that I don't want to using class
   * FINTE for SendDataFn
   */
  virtual bool SendData(const char *data, const int32_t len) = 0;

protected:
  std::string channel_;
};
} // namespace transport
