#include "iwr6843_read.h"

#include <sys/ioctl.h>

#include <bitset>

namespace radar {

// A magic word that used to found the front of a message
const char kMagicWord[8] = {0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};

double NowSec() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() /
         1000.0;
}

Reader_IWR6843::Reader_IWR6843(const char *cli_sock, const speed_t cli_baudrate,
                               const char *data_sock,
                               const speed_t data_baudrate) {
  static_assert(sizeof(float) == 4, "Size incorrect for float");
  /**
   * The iwr6843 using two serial port. 1 for data transfer, 1 for control purpose 
   */
  cli_fd_ = OpenSerial(cli_sock, cli_baudrate, &cli_tty_);
  data_fd_ = OpenSerial(data_sock, data_baudrate, &data_tty_);

  memset(acum_buf, 0, sizeof(acum_buf));
}

Reader_IWR6843::~Reader_IWR6843() {
  char stop_cmd[] = "sensorStop\n";
  WriteToSerial(cli_fd_, stop_cmd, sizeof(stop_cmd));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  close(cli_fd_);
  close(data_fd_);
}

void Reader_IWR6843::Setup(const char *config_file) {
  std::ifstream f(config_file);
  if (!f.is_open()) {
    std::cerr << "[iwr6843] Error while opening the config file " << config_file
              << std::endl;
  } else {
    std::string buf;
    while (std::getline(f, buf)) {
      if (!buf.empty() && buf[0] == '%') {
        continue;
      }
      boost::algorithm::trim(buf);
      buf += '\n';
      if (!WriteToSerial(cli_fd_, buf.c_str(), buf.size())) {
        std::cerr << "Error while writing config" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    f.close();
  }
  std::cout << "[IWR6843] Done Setup" << std::endl;
}

int Reader_IWR6843::ReadRawData(char *read_buf, const int buf_size) {
  int read_size = std::min(kMaxBufSize, buf_size);
  int len = ReadFromSerial(data_fd_, buf, read_size);
  memcpy(read_buf, buf, len);
  return len;
}

bool Reader_IWR6843::ReadRawRadarResult(RawRadarResult *result) {
  static int32_t raw_seq = 0;
  result->Clear();
  result->set_seq(raw_seq);
  result->set_timestamp(NowSec());
  result->set_model(Model::IWR6843);
  raw_seq++;
  ReadFullPacketToBuff();

  MmwDemo_output_message_header *hdr =
      reinterpret_cast<MmwDemo_output_message_header *>(acum_buf);
  int total_len = hdr->totalPacketLen;
  *(result->mutable_data()) = {acum_buf, acum_buf + total_len};
  acum_len -= total_len;
  if (acum_len > 0) {
    memmove(acum_buf, acum_buf + total_len, acum_len);
  }
  return true;
}

bool Reader_IWR6843::ReadParsedRadarResult(RadarResult *result) {
  static int32_t parsed_seq = 0;
  result->Clear();
  result->set_seq(parsed_seq);
  result->set_timestamp(NowSec());
  result->set_model(Model::IWR6843);
  parsed_seq++;

  ReadFullPacketToBuff();
  // Parse Header
  MmwDemo_output_message_header *hdr =
      reinterpret_cast<MmwDemo_output_message_header *>(acum_buf);
  // std::cout << "version: " << hdr->version << std::endl;
  // std::cout << "frameNumber: " << hdr->frameNumber << std::endl;
  // std::cout << "numTLVs:  " << hdr->numTLVs << std::endl;
  // std::cout << "numDetectedObj:  " << hdr->numDetectedObj << std::endl;
  // std::cout << "totalPacketLen: " << hdr->totalPacketLen << std::endl;
  int total_len = hdr->totalPacketLen;
  int n = hdr->numTLVs;
  int m = hdr->numDetectedObj;
  result->mutable_object()->Reserve(m);
  int ridx = sizeof(MmwDemo_output_message_header);
  // Reading message from buf
  for (int i = 0; i < n; ++i) {
    TypeNLen *t = reinterpret_cast<TypeNLen *>(acum_buf + ridx);
    ridx += sizeof(TypeNLen);
    if (t->tly_type == 1) {
      for (int j = 0; j < m; ++j) {
        TlvObject *obj = reinterpret_cast<TlvObject *>(acum_buf + ridx);
        auto ptr = result->add_object();
        ptr->set_x(obj->x);
        ptr->set_y(obj->y);
        ptr->set_z(obj->z);
        ptr->set_vel(obj->vel);
        ridx += sizeof(TlvObject);
      }
      break;
    } else {
      ridx += t->tly_len;
    }
  }
  acum_len -= total_len;
  // Remove the readed data from buffer
  if (acum_len > 0) {
    memmove(acum_buf, acum_buf + total_len, acum_len);
  }
  return true;
}

int Reader_IWR6843::OpenSerial(const char *sock, const speed_t baudrate,
                               termios *tty) {
  int serial_port = open(sock, O_RDWR | O_NOCTTY);
  if (serial_port < 0) {
    printf("Error %i from open: %s\n", errno, strerror(errno));
    exit(1);
  }
  if (tcgetattr(serial_port, tty) < 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    exit(1);
  }

  tty->c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty->c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in
                           // communication (most common)
  tty->c_cflag |= CS8;     // 8 bits per byte (most common)
  tty->c_cflag &=
      ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty->c_cflag |=
      CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
  // tty->c_cflag=0x18b2;
  tty->c_lflag &= ~ICANON;
  tty->c_lflag &= ~ECHO;   // Disable echo
  tty->c_lflag &= ~ECHOE;  // Disable erasure
  tty->c_lflag &= ~ECHONL; // Disable new-line echo
  tty->c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP
  tty->c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                    ICRNL); // Disable any special handling of received bytes
  tty->c_iflag &= ~INPCK;

  tty->c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
                          // (e.g. newline chars)
  tty->c_oflag &=
      ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty->c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as
                         // soon as any data is received.
  tty->c_cc[VMIN] = 0;
  cfsetispeed(tty, baudrate);
  cfsetospeed(tty, baudrate);

  if (tcsetattr(serial_port, TCSANOW, tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    exit(1);
  }
  // Required for tcflush
  sleep(2);
  char rts = TIOCM_RTS;
  char dtr = TIOCM_DTR;
  ioctl(serial_port, TCFLSH, 2);
  ioctl(serial_port, TIOCMBIS, &dtr);
  ioctl(serial_port, TIOCMBIS, &rts);

  // Trying to clean the buffer in the serial channel
  if (tcflush(serial_port, TCIOFLUSH) != 0) {
    printf("Error %i from tcflush: %s\n", errno, strerror(errno));
    exit(1);
  }

  return serial_port;
}

bool Reader_IWR6843::WriteToSerial(const int fd, const char *data, size_t len) {
  int ret = write(fd, data, len);
  return ret != -1;
}

int Reader_IWR6843::ReadFromSerial(const int fd, char *buf, size_t buf_size) {
  int len = read(fd, buf, buf_size);
  if (len < 0) {
    printf("Error %i from read: %s\n", errno, strerror(errno));
    exit(1);
  }
  return len;
}

int Reader_IWR6843::CheckMagic(const char *buf, size_t len) const {
  constexpr int nc = 8;
  for (int i = 0; i + nc < len; ++i) {
    for (int j = 0; j < nc; ++j) {
      if (buf[i + j] != kMagicWord[j]) {
        i = i + j;
        break;
      }
      if (j + 1 == nc) {
        return i;
      }
    }
  }
  return -1;
}

void Reader_IWR6843::ReadFullPacketToBuff() {
  bool magic = false;

  // If no magic, read until have magic
  while (!magic) {
    int offset = CheckMagic(acum_buf, acum_len);
    if (offset >= 0) {
      magic = true;
      memmove(acum_buf, acum_buf + offset, acum_len - offset);
      acum_len = acum_len - offset;
    } else {
      int len = ReadFromSerial(data_fd_, buf, kMaxBufSize);
      // assume new + old will not over the max
      memcpy(acum_buf + acum_len, buf, len);
      acum_len += len;
    }
  }
  // Ensure read the entire header
  while (acum_len < sizeof(MmwDemo_output_message_header)) {
    int len =
        ReadFromSerial(data_fd_, acum_buf + acum_len, kMaxBufSize - acum_len);
    acum_len += len;
  }

  MmwDemo_output_message_header *hdr =
      reinterpret_cast<MmwDemo_output_message_header *>(acum_buf);
  int total_len = hdr->totalPacketLen;
  while (acum_len < total_len) {
    int len =
        ReadFromSerial(data_fd_, acum_buf + acum_len, kMaxBufSize - acum_len);
    acum_len += len;
  }
}

} // namespace radar
