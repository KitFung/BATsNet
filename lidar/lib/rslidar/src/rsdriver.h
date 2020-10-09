/* -*- mode: C++ -*- */
/*
 *  Copyright (C) 2012 Austin Robot Technology, Jack O'Quin
 *	Copyright (C) 2017 Robosense, Tony Zhang
 *
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** \file
 *
 *  ROS driver interface for the RSLIDAR 3D LIDARs
 */
#ifndef _RSDRIVER_H_
#define _RSDRIVER_H_

#include <boost/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>

#include "input.h"

#include "proto/rslidar.pb.h"

namespace rslidar {
class rslidarDriver {
public:
  /**
   * @brief rslidarDriver
   * @param node          raw packet output topic
   * @param private_nh    通过这个节点传参数
   */
  rslidarDriver(Conf conf);

  ~rslidarDriver() { running_ = false; }

  bool poll(Scan *scan);
  // void difopPoll(void);

private:
  // configuration parameters
  struct {
    std::string frame_id; ///< tf frame ID
    std::string model;    ///< device model name
    int npackets;         ///< number of packets to collect
    double rpm;           ///< device rotation rate (RPMs)
    double time_offset;   ///< time in seconds added to each  time stamp
    int cut_angle;
  } config_;

  boost::shared_ptr<Input> msop_input_;
  boost::shared_ptr<Input> difop_input_;

  boost::shared_ptr<boost::thread> difop_thread_;
  bool running_ = true;
  int seq = 0;
};

} // namespace rslidar

#endif
