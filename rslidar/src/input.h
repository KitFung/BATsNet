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

#ifndef __RSLIDAR_INPUT_H_
#define __RSLIDAR_INPUT_H_

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pcap.h>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>

#include <iostream>

#include "proto_gen/rslidar.pb.h"

namespace rslidar {
static uint16_t MSOP_DATA_PORT_NUMBER = 6699; // rslidar default data port on PC
static uint16_t DIFOP_DATA_PORT_NUMBER =
    7788; // rslidar default difop data port on PC
          /**
           *  从在线的网络数据或离线的网络抓包数据（pcap文件）中提取出lidar的原始数据，即packet数据包
           * @brief The Input class,
           *
           * @param private_nh  一个NodeHandled,用于通过节点传递参数
           * @param port
           * @returns 0 if successful,
           *          -1 if end of file
           *          >0 if incomplete packet (is this possible?)
           */
class Input {
public:
  Input(Conf conf, uint16_t port);

  virtual ~Input() {}

  virtual int getPacket(Packet *pkt, const double time_offset) = 0;

  int getRpm(void);
  int getReturnMode(void);
  bool getUpdateFlag(void);
  void clearUpdateFlag(void);

protected:
  Conf conf_;
  uint16_t port_;
  std::string devip_str_;
  int cur_rpm_;
  int return_mode_;
  bool npkt_update_flag_;
  int seq = 0;
};

/** @brief Live rslidar input from socket. */
class InputSocket : public Input {
public:
  InputSocket(Conf conf, uint16_t port = MSOP_DATA_PORT_NUMBER);

  virtual ~InputSocket();

  virtual int getPacket(Packet *pkt, const double time_offset);

private:
private:
  int sockfd_;
  in_addr devip_;
};

} // namespace rslidar

#endif // __RSLIDAR_INPUT_H
