/*
 *  Copyright (C) 2007 Austin Robot Technology, Patrick Beeson
 *  Copyright (C) 2009-2012 Austin Robot Technology, Jack O'Quin
 *	Copyright (C) 2017 Robosense, Tony Zhang
 *
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** \file
 *
 *  ROS driver implementation for the RILIDAR 3D LIDARs
 */
#include "rsdriver.h"

namespace rslidar {
static const unsigned int POINTS_ONE_CHANNEL_PER_SECOND = 18000;
static const unsigned int BLOCKS_ONE_CHANNEL_PER_PKT = 12;

rslidarDriver::rslidarDriver(const char *broker_ip, const int broker_port,
                             Conf conf) {
  collector_ = boost::shared_ptr<data_collecter::DataCollector>(
      new data_collecter::DataCollector(broker_ip, broker_port));

  // get model name, validate string, determine packet rate
  config_.model = conf.model();
  double packet_rate; // packet frequency (Hz)
  std::string model_full_name;

  // product model
  if (config_.model == "RS16") {
    // for 0.18 degree horizontal angle resolution
    // packet_rate = 840;
    // for 0.2 degree horizontal angle resolution
    packet_rate = 750;
    model_full_name = "RS-LiDAR-16";
  } else if (config_.model == "RS32") {
    // for 0.18 degree horizontal angle resolution
    // packet_rate = 1690;
    // for 0.2 degree horizontal angle resolution
    packet_rate = 1500;
    model_full_name = "RS-LiDAR-32";
  } else if (config_.model == "RSBPEARL") {
    packet_rate = 1500;
    model_full_name = "RSBPEARL";
  } else if (config_.model == "RSBPEARL_MINI") {
    packet_rate = 1500;
    model_full_name = "RSBPEARL_MINI";
  } else {
    std::cerr << "[driver] unknown LIDAR model: " << config_.model << std::endl;
    packet_rate = 2600.0;
  }
  std::string deviceName(std::string("Robosense ") + model_full_name);

  config_.rpm = 600;
  if (conf.has_rpm()) {
    config_.rpm = conf.rpm();
  }

  double frequency = (config_.rpm / 60.0); // expected Hz rate

  // default number of packets for each scan is a single revolution
  // (fractions rounded up)

  int npackets = (int)ceil(packet_rate / frequency);
  config_.npackets = npackets;
  if (conf.has_npackets()) {
    config_.npackets = conf.npackets();
  }

  std::cout << "[driver] publishing " << config_.npackets << " packets per scan"
            << std::endl;

  int msop_udp_port = (int)MSOP_DATA_PORT_NUMBER;
  if (conf.has_msop_port()) {
    msop_udp_port = conf.msop_port();
  }
  int difop_udp_port = (int)DIFOP_DATA_PORT_NUMBER;
  if (conf.has_difop_port()) {
    difop_udp_port = conf.difop_port();
  }

  double cut_angle = -0.01;
  if (conf.has_cut_angle()) {
    cut_angle = conf.cut_angle();
  }

  if (cut_angle < 0.0) {
    std::cout << "[driver] Cut at specific angle feature deactivated."
              << std::endl;
  } else if (cut_angle < 360) {
    std::cout << "[driver] Cut at specific angle feature activated. "
                 "Cutting rslidar points always at "
              << cut_angle << " degree." << std::endl;
  } else {
    std::cerr
        << "[driver] cut_angle parameter is out of range. Allowed range is "
        << "between 0.0 and 360 negative values to deactivate this feature."
        << std::endl;
    cut_angle = -0.01;
  }

  // Convert cut_angle from radian to one-hundredth degree,
  // which is used in rslidar packets
  config_.cut_angle = static_cast<int>(cut_angle * 100);

  // read data from live socket
  msop_input_.reset(new rslidar::InputSocket(conf, msop_udp_port));
  difop_input_.reset(new rslidar::InputSocket(conf, difop_udp_port));

  // raw packet output topic
  output_packets_topic_ = conf.output_packets_topic();
  output_difop_topic_ = conf.output_difop_topic();

  difop_thread_ = boost::shared_ptr<boost::thread>(
      new boost::thread(boost::bind(&rslidarDriver::difopPoll, this)));
}

/** poll the device
 *
 *  @returns true unless end of file reached
 */
bool rslidarDriver::poll(void) { // Allocate a new shared pointer for zero-copy
                                 // sharing with other nodelets.
  Scan scan;
  // Since the rslidar delivers data at a very high rate, keep
  // reading and publishing scans as fast as possible.
  if (config_.cut_angle >= 0) // Cut at specific angle feature enabled
  {
    scan.mutable_packet()->Reserve(config_.npackets);
    while (true) {
      Packet *tmp_packet = scan.add_packet();
      while (true) {
        int rc = msop_input_->getPacket(tmp_packet, config_.time_offset);
        if (rc == 0)
          break; // got a full packet?
        if (rc < 0)
          return false; // end of file reached?
      }
      static int ANGLE_HEAD =
          -36001; // note: cannot be set to -1, or stack smashing
      static int last_azimuth = ANGLE_HEAD;
      int azimuth = 256 * tmp_packet->data()[44] + tmp_packet->data()[45];
      // int azimuth = *( (u_int16_t*) (&tmp_packet.data[azimuth_data_pos]));
      // Handle overflow 35999->0
      if (azimuth < last_azimuth) {
        last_azimuth -= 36000;
      }
      // Check if currently passing cut angle
      if (last_azimuth != ANGLE_HEAD && last_azimuth < config_.cut_angle &&
          azimuth >= config_.cut_angle) {
        last_azimuth = azimuth;
        break; // Cut angle passed, one full revolution collected
      }
      last_azimuth = azimuth;
    }
  } else // standard behaviour
  {
    if (difop_input_->getUpdateFlag()) {
      int packets_rate =
          ceil(POINTS_ONE_CHANNEL_PER_SECOND / BLOCKS_ONE_CHANNEL_PER_PKT);
      int mode = difop_input_->getReturnMode();
      if (config_.model == "RS16" && (mode == 1 || mode == 2)) {
        packets_rate = ceil(packets_rate / 2);
      } else if ((config_.model == "RS32" || config_.model == "RSBPEARL" ||
                  config_.model == "RSBPEARL_MINI") &&
                 (mode == 0)) {
        packets_rate = packets_rate * 2;
      }
      config_.rpm = difop_input_->getRpm();
      config_.npackets = ceil(packets_rate * 60 / config_.rpm);

      difop_input_->clearUpdateFlag();

      std::cout << "[driver] update npackets. rpm: " << config_.rpm
                << ", npkts: " << config_.npackets << std::endl;
    }
    while (scan.packet_size() < config_.npackets) {
      scan.add_packet();
    }

    for (int i = 0; i < config_.npackets; ++i) {
      while (true) {
        // keep reading until full packet received
        int rc =
            msop_input_->getPacket(scan.mutable_packet(i), config_.time_offset);
        if (rc == 0)
          break; // got a full packet?
        if (rc < 0)
          return false; // end of file reached?
      }
    }
  }
  // publish message using time of last packet read
  scan.set_seq(seq);
  scan.set_timestamp(scan.packet(scan.packet_size() - 1).timestamp());
  seq++;
  auto data_str = scan.SerializeAsString();
  bool flag = collector_->SendData(output_packets_topic_.c_str(),
                                   data_str.c_str(), data_str.size());
  // std::cout << scan.packet_size() << "|" << config_.npackets << std::endl;
  // std::cout << "Send Data to " << output_packets_topic_
  //           << "  size: " << scan.SerializeAsString().size() << " ret: " <<
  //           flag
  //           << std::endl;
  return true;
}

void rslidarDriver::difopPoll(void) {
  // reading and publishing scans as fast as possible.
  Packet difop_packet;
  while (running_) {
    // keep reading
    difop_packet.Clear();
    int rc = difop_input_->getPacket(&difop_packet, config_.time_offset);
    if (rc == 0) {
      auto data_str = difop_packet.SerializeAsString();
      collector_->SendData(output_difop_topic_.c_str(), data_str.c_str(),
                           data_str.size());
    }
    if (rc < 0)
      return; // end of file reached?
    boost::this_thread::sleep_for(boost::chrono::microseconds(100));
  }
}

} // namespace rslidar
