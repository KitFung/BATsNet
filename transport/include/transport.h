namespace transport {
class Transport {
public:
  template <typename T>
  void Send(const void *buf, const int buf_size);

};
} // namespace transport