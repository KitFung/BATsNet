// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <errno.h>   // Error integer and strerror() function
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()

// Cpp library headers
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <boost/algorithm/string.hpp>

#include "proto_gen/radar.pb.h"

namespace radar {
constexpr int kMaxBufSize = 1 << 16;

#pragma pack(push, 4)
struct MmwDemo_output_message_header {
  uint16_t magicWord[4];
  uint32_t version;
  uint32_t totalPacketLen;
  uint32_t platform;
  uint32_t frameNumber;
  uint32_t timeCpuCycles;
  uint32_t numDetectedObj;
  uint32_t numTLVs;
  uint32_t subFrameNumber;
};
struct TypeNLen {
  uint32_t tly_type;
  uint32_t tly_len;
};
struct TlvObject {
  float x;
  float y;
  float z;
  float vel;
};
#pragma pack(pop)

class Reader_IWR6843 {
public:
  Reader_IWR6843(const char *cli_sock, const speed_t cli_baudrate,
                 const char *data_sock, const speed_t data_baudrate);
  ~Reader_IWR6843();
  void Setup(const char *config_file);
  int ReadRawData(char *read_buf, const int buf_size);
  bool ReadRawRadarResult(RawRadarResult *result);
  bool ReadParsedRadarResult(RadarResult *result);

private:
  int OpenSerial(const char *sock, const speed_t baudrate, termios *tty);
  bool WriteToSerial(const int fd, const char *data, size_t len);
  int ReadFromSerial(const int fd, char *buf, size_t buf_size);

  int CheckMagic(const char *buf, size_t len) const;
  void ReadFullPacketToBuff();

  char buf[kMaxBufSize];
  char acum_buf[kMaxBufSize];
  int acum_len = 0;

  termios cli_tty_;
  termios data_tty_;
  int cli_fd_;
  int data_fd_;
};

} // namespace radar