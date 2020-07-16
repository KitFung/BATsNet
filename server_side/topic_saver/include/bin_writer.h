#include <string.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <openssl/sha.h>

namespace topic_saver {
constexpr int kMaxBuf = (1 << 20) * 10; // 1 MB

struct DataBuffer {
  explicit DataBuffer(const std::string &topic) : topic_name(topic) {}

  std::string topic_name;
  int32_t message_len = 0;
  int32_t buf_len = 0;
  char buf[kMaxBuf];
  uint32_t each_len[kMaxBuf];

  void Append(const char *data, const int len) {
    memcpy(buf + buf_len, data, len * sizeof(char));
    buf_len += len;
    each_len[message_len] = len;
    message_len += 1;
  }

  inline size_t ByteSize() const { return buf_len; }
};

std::string GetNowTimeStramp();
// Write Format
// | 4 byte   | 4 byte  | dynamic length | 4 byte | ....
//  total msg | msg len | content        | msg len | ...

class BinWriter {
public:
  explicit BinWriter(const char *save_folder);
~BinWriter();

  void AddTask(std::shared_ptr<DataBuffer> data);
  void WorkingLoop();

private:
  void Write(const DataBuffer *data);
  // void GenerateHashSum();

  std::string save_folder_;
  char tmp_buf[kMaxBuf];
  bool running_ = true;
  std::mutex mtx_;
  std::queue<std::shared_ptr<DataBuffer>> bufs_;
  std::thread working_thread_;
};
} // namespace topic_saver