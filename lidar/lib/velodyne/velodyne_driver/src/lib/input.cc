// Copyright (C) 2007, 2009, 2010, 2015 Austin Robot Technology, Patrick Beeson,
// Jack O'Quin All rights reserved.
//
// Software License Agreement (BSD License 2.0)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//  * Neither the name of {copyright_holder} nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/** \file
 *
 *  Input classes for the Velodyne HDL-64E 3D LIDAR:
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

#include <arpa/inet.h>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <velodyne_driver/input.h>
#include <velodyne_driver/time_conversion.hpp>

namespace velodyne_driver {
static const size_t packet_size = sizeof(uint8_t) * 1206;

////////////////////////////////////////////////////////////////////////
// Input base class implementation
////////////////////////////////////////////////////////////////////////

/** @brief constructor
 *
 *  @param private_nh ROS private handle for calling node.
 *  @param port UDP port number.
 */
Input::Input(const velodyne::VelodyneDriverConf &conf, uint16_t port)
    : conf_(conf), port_(port) {
  devip_str_ = conf.device_ip();
  gps_time_ = conf.gps_time();
  if (!devip_str_.empty())
    std::cerr << "Only accepting packets from IP address: " << devip_str_
              << std::endl;
}

////////////////////////////////////////////////////////////////////////
// InputSocket class implementation
////////////////////////////////////////////////////////////////////////

/** @brief constructor
 *
 *  @param private_nh ROS private handle for calling node.
 *  @param port UDP port number
 */
InputSocket::InputSocket(const velodyne::VelodyneDriverConf &conf,
                         uint16_t port)
    : Input(conf, port) {
  sockfd_ = -1;

  if (!devip_str_.empty()) {
    inet_aton(devip_str_.c_str(), &devip_);
  }

  // connect to Velodyne UDP port
  std::cout << "Opening UDP socket: port " << port << std::endl;
  sockfd_ = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd_ == -1) {
    perror("socket"); // TODO: ROS_ERROR errno
    return;
  }

  sockaddr_in my_addr;                  // my address information
  memset(&my_addr, 0, sizeof(my_addr)); // initialize to zeros
  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(port);       // port in network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill in my IP

  if (bind(sockfd_, (sockaddr *)&my_addr, sizeof(sockaddr)) == -1) {
    perror("bind"); // TODO: ROS_ERROR errno
    return;
  }

  if (fcntl(sockfd_, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
    perror("non-block");
    return;
  }

  std::cout << "Velodyne socket fd is " << sockfd_ << std::endl;
}

/** @brief destructor */
char buf[packet_size];
InputSocket::~InputSocket(void) { (void)close(sockfd_); }

/** @brief Get one velodyne packet. */
int InputSocket::getPacket(velodyne::VelodynePacket *pkt,
                           const double time_offset) {
  double time1 = NowSec();

  struct pollfd fds[1];
  fds[0].fd = sockfd_;
  fds[0].events = POLLIN;
  static const int POLL_TIMEOUT = 1000; // one second (in msec)

  sockaddr_in sender_address;
  socklen_t sender_address_len = sizeof(sender_address);

  while (true) {
    // Unfortunately, the Linux kernel recvfrom() implementation
    // uses a non-interruptible sleep() when waiting for data,
    // which would cause this method to hang if the device is not
    // providing data.  We poll() the device first to make sure
    // the recvfrom() will not block.
    //
    // Note, however, that there is a known Linux kernel bug:
    //
    //   Under Linux, select() may report a socket file descriptor
    //   as "ready for reading", while nevertheless a subsequent
    //   read blocks.  This could for example happen when data has
    //   arrived but upon examination has wrong checksum and is
    //   discarded.  There may be other circumstances in which a
    //   file descriptor is spuriously reported as ready.  Thus it
    //   may be safer to use O_NONBLOCK on sockets that should not
    //   block.

    // poll() until input available
    do {
      int retval = poll(fds, 1, POLL_TIMEOUT);
      if (retval < 0) // poll() error?
      {
        if (errno != EINTR)
          std::cerr << "poll() error: " << strerror(errno) << std::endl;
        return -1;
      }
      if (retval == 0) // poll() timeout?
      {
        std::cerr << "Velodyne poll() timeout" << std::endl;
        return -1;
      }
      if ((fds[0].revents & POLLERR) || (fds[0].revents & POLLHUP) ||
          (fds[0].revents & POLLNVAL)) // device error?
      {
        std::cerr << "poll() reports Velodyne error" << std::endl;
        return -1;
      }
    } while ((fds[0].revents & POLLIN) == 0);

    // Receive packets that should now be available from the
    // socket using a blocking read.
    pkt->mutable_data()->resize(packet_size);
    ssize_t nbytes = recvfrom(sockfd_, &buf[0], packet_size, 0,
                              (sockaddr *)&sender_address, &sender_address_len);
    *(pkt->mutable_data()) = {&buf[0], &buf[0] + packet_size};

    if (nbytes < 0) {
      if (errno != EWOULDBLOCK) {
        perror("recvfail");
        std::cerr << "recvfail" << std::endl;
        return -1;
      }
    } else if ((size_t)nbytes == packet_size) {
      // read successful,
      // if packet is not from the lidar scanner we selected by IP,
      // continue otherwise we are done
      if (devip_str_ != "" && sender_address.sin_addr.s_addr != devip_.s_addr)
        continue;
      else
        break; // done
    }

    std::cerr << "incomplete Velodyne packet read: " << nbytes << " bytes"
              << std::endl;
  }

  if (!gps_time_) {
    // Average the times at which we begin and end reading.  Use that to
    // estimate when the scan occurred. Add the time offset.
    double time2 = NowSec();
    pkt->set_stamp((time2 + time1) / 2.0 + time_offset);
  } else {
    // time for each packet is a 4 byte uint located starting at offset 1200 in
    // the data packet
    pkt->set_stamp(
        timeFromGpsTimestamp(reinterpret_cast<uint8_t *>(&(buf[1200]))));
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////
// InputPCAP class implementation
////////////////////////////////////////////////////////////////////////

/** @brief constructor
 *
 *  @param private_nh ROS private handle for calling node.
 *  @param port UDP port number
 *  @param packet_rate expected device packet frequency (Hz)
 *  @param filename PCAP dump file name
 */
InputPCAP::InputPCAP(const velodyne::VelodyneDriverConf &conf, uint16_t port,
                     double packet_rate, std::string filename, bool read_once,
                     bool read_fast, double repeat_delay)
    : Input(conf, port), packet_rate_(packet_rate), filename_(filename) {
  pcap_ = NULL;
  empty_ = true;

  // get parameters using private node handle
  read_once_ = conf.read_once();
  read_fast_ = conf.read_fast();
  repeat_delay_ = conf.repeat_delay();

  if (read_once_)
    std::cout << "Read input file only once." << std::endl;
  if (read_fast_)
    std::cout << "Read input file as quickly as possible." << std::endl;
  if (repeat_delay_ > 0.0)
    std::cout << "Delay " << repeat_delay_
              << " seconds before repeating input file." << std::endl;

  // Open the PCAP dump file
  std::cout << "Opening PCAP file " << filename_ << std::endl;
  if ((pcap_ = pcap_open_offline(filename_.c_str(), errbuf_)) == NULL) {
    std::cerr << "Error opening Velodyne socket dump file." << std::endl;
    return;
  }

  std::stringstream filter;
  if (devip_str_ != "") // using specific IP?
  {
    filter << "src host " << devip_str_ << " && ";
  }
  filter << "udp dst port " << port;
  pcap_compile(pcap_, &pcap_packet_filter_, filter.str().c_str(), 1,
               PCAP_NETMASK_UNKNOWN);
}

/** destructor */
InputPCAP::~InputPCAP(void) { pcap_close(pcap_); }

/** @brief Get one velodyne packet. */
int InputPCAP::getPacket(velodyne::VelodynePacket *pkt,
                         const double time_offset) {
  struct pcap_pkthdr *header;
  const u_char *pkt_data;

  while (true) {
    int res;
    if ((res = pcap_next_ex(pcap_, &header, &pkt_data)) >= 0) {
      // Skip packets not for the correct port and from the
      // selected IP address.
      if (0 == pcap_offline_filter(&pcap_packet_filter_, header, pkt_data))
        continue;

      // Keep the reader from blowing through the file.
      if (read_fast_ == false) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(packet_rate_ / 1000.0)));
      }
      const char *pkt_data_c = reinterpret_cast<const char *>(pkt_data);
      *(pkt->mutable_data()) =
          std::string(pkt_data_c + 42, pkt_data_c + 42 + packet_size);
      pkt->set_stamp(NowSec()); // time_offset not considered here, as no
                                // synchronization required
      empty_ = false;
      return 0; // success
    }

    if (empty_) // no data in file?
    {
      std::cerr << "Error " << res
                << " reading Velodyne packet: " << pcap_geterr(pcap_)
                << std::endl;
      return -1;
    }

    if (read_once_) {
      std::cout << "end of file reached -- done reading." << std::endl;
      return -1;
    }

    if (repeat_delay_ > 0.0) {
      std::cout << "end of file reached -- delaying " << repeat_delay_
                << " seconds." << std::endl;
      usleep(int(repeat_delay_ * 1000000.0));
    }

    std::cout << "replaying Velodyne dump file" << std::endl;

    // I can't figure out how to rewind the file, because it
    // starts with some kind of header.  So, close the file
    // and reopen it with pcap.
    pcap_close(pcap_);
    pcap_ = pcap_open_offline(filename_.c_str(), errbuf_);
    empty_ = true; // maybe the file disappeared?
  }                // loop back and try again
}

} // namespace velodyne_driver
