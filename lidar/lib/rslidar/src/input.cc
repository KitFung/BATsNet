/*
 *  Copyright (C) 2007 Austin Robot Technology, Patrick Beeson
 *  Copyright (C) 2009, 2010 Austin Robot Technology, Jack O'Quin
 *  Copyright (C) 2015, Jack O'Quin
 *	Copyright (C) 2017, Robosense, Tony Zhang
 *
 *
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** \file
 *
 *  Input classes for the RSLIDAR RS-16 3D LIDAR:
 *
 *     Input -- base class used to access the data independently of
 *              its source
 *
 *     InputSocket -- derived class reads live data from the device
 *              via a UDP socket
 *
 *     InputPCAP -- derived class provides a similar interface from a
 *              PCAP dump
 */
#include "input.h"

extern volatile sig_atomic_t flag;
namespace rslidar {
static const size_t packet_size = 1248;

////////////////////////////////////////////////////////////////////////
// Input base class implementation
////////////////////////////////////////////////////////////////////////

/** @brief constructor
 *
 *  @param private_nh ROS private handle for calling node.
 *  @param port UDP port number.
 */
Input::Input(Conf conf, uint16_t port) : conf_(conf), port_(port) {
  npkt_update_flag_ = false;
  cur_rpm_ = 600;
  return_mode_ = 1;

  devip_str_ = conf.device_ip();
  if (!devip_str_.empty()) {
    std::cout << "[driver][input] accepting packets from IP address: "
              << devip_str_ << std::endl;
  }
}

int Input::getRpm(void) { return cur_rpm_; }

int Input::getReturnMode(void) { return return_mode_; }

bool Input::getUpdateFlag(void) { return npkt_update_flag_; }

void Input::clearUpdateFlag(void) { npkt_update_flag_ = false; }
////////////////////////////////////////////////////////////////////////
// InputSocket class implementation
////////////////////////////////////////////////////////////////////////

/** @brief constructor
 *
 *  @param private_nh ROS private handle for calling node.
 *  @param port UDP port number
 */
InputSocket::InputSocket(Conf conf, uint16_t port) : Input(conf, port) {
  sockfd_ = -1;

  if (!devip_str_.empty()) {
    inet_aton(devip_str_.c_str(), &devip_);
  }

  std::cout << "[driver][socket] Opening UDP socket: port " << port
            << std::endl;
  sockfd_ = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd_ == -1) {
    std::cerr << "[driver][socket] create socket fail" << std::endl;
    return;
  }

  int opt = 1;
  if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt,
                 sizeof(opt))) {
    std::cerr << "[driver][socket] setsockopt fail" << std::endl;
    return;
  }

  sockaddr_in my_addr;                  // my address information
  memset(&my_addr, 0, sizeof(my_addr)); // initialize to zeros
  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(port);       // port in network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill in my IP

  if (bind(sockfd_, (sockaddr *)&my_addr, sizeof(sockaddr)) == -1) {
    std::cerr << "[driver][socket] socket bind fail" << std::endl;
    return;
  }

  if (fcntl(sockfd_, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
    std::cerr << "[driver][socket] fcntl fail" << std::endl;
    return;
  }
}

/** @brief destructor */
InputSocket::~InputSocket(void) { (void)close(sockfd_); }

double NowSec() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() *
         1000.0;
}
/** @brief Get one rslidar packet. */
char buf[packet_size];
int InputSocket::getPacket(Packet *pkt, const double time_offset) {
  double time1 = NowSec();
  struct pollfd fds[1];
  fds[0].fd = sockfd_;
  fds[0].events = POLLIN;
  static const int POLL_TIMEOUT = 1000; // one second (in msec)
  sockaddr_in sender_address;
  socklen_t sender_address_len = sizeof(sender_address);
  while (flag == 1) {
    // Receive packets that should now be available from the
    // socket using a blocking read.
    // poll() until input available
    do {
      int retval = poll(fds, 1, POLL_TIMEOUT);
      if (retval < 0) // poll() error?
      {
        if (errno != EINTR) {
          std::cerr << "[driver][socket] poll() error: " << strerror(errno)
                    << std::endl;
        }
        return 1;
      }
      if (retval == 0) // poll() timeout?
      {
        std::cerr << "[driver][socket] Rslidar poll() timeout" << std::endl;

        char buffer_data[8] = "re-con";
        memset(&sender_address, 0, sender_address_len); // initialize to zeros
        sender_address.sin_family = AF_INET;            // host byte order
        sender_address.sin_port = htons(
            MSOP_DATA_PORT_NUMBER); // port in network byte order, set any value
        sender_address.sin_addr.s_addr =
            devip_.s_addr; // automatically fill in my IP
        sendto(sockfd_, &buffer_data, strlen(buffer_data), 0,
               (sockaddr *)&sender_address, sender_address_len);
        return 1;
      }
      if ((fds[0].revents & POLLERR) || (fds[0].revents & POLLHUP) ||
          (fds[0].revents & POLLNVAL)) // device error?
      {
        std::cerr << "[driver][socket] poll() reports Rslidar error"
                  << std::endl;
        return 1;
      }
    } while ((fds[0].revents & POLLIN) == 0);
    pkt->mutable_data()->resize(packet_size);
    ssize_t nbytes = recvfrom(sockfd_, &buf[0], packet_size, 0,
                              (sockaddr *)&sender_address, &sender_address_len);
    *(pkt->mutable_data()) = {&buf[0], &buf[0] + packet_size};
    if (nbytes < 0) {
      if (errno != EWOULDBLOCK) {
        std::cerr << "[driver][socket] recvfail" << std::endl;
        return 1;
      }
    } else if ((size_t)nbytes == packet_size) {
      if (devip_str_ != "" && sender_address.sin_addr.s_addr != devip_.s_addr) {
        continue;
      } else {
        break; // done
      }
    }

    std::cerr << "[driver][socket] incomplete rslidar packet read: " << nbytes
              << " bytes" << std::endl;
  }
  if (flag == 0) {
    abort();
  }
  auto &data = pkt->data();
  if (data[0] == 0xA5 && data[1] == 0xFF && data[2] == 0x00 &&
      data[3] == 0x5A) { // difop
    int rpm = (data[8] << 8) | data[9];
    int mode = 1;

    if ((data[45] == 0x08 && data[46] == 0x02 && data[47] >= 0x09) ||
        (data[45] > 0x08) || (data[45] == 0x08 && data[46] > 0x02)) {
      if (data[300] != 0x01 && data[300] != 0x02) {
        mode = 0;
      }
    }

    if (cur_rpm_ != rpm || return_mode_ != mode) {
      cur_rpm_ = rpm;
      return_mode_ = mode;

      npkt_update_flag_ = true;
    }
  }

  // Average the times at which we begin and end reading.  Use that to
  // estimate when the scan occurred. Add the time offset.
  double time2 = NowSec();

  pkt->set_timestamp((time2 + time1) / 2.0 + time_offset);
  pkt->set_seq(seq);

  seq++;
  return 0;
}

} // namespace rslidar
