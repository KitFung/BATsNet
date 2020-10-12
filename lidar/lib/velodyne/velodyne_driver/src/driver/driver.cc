// Copyright (C) 2007, 2009-2012 Austin Robot Technology, Patrick Beeson, Jack
// O'Quin All rights reserved.
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
 *  ROS driver implementation for the Velodyne 3D LIDARs
 */

#include <cmath>
#include <string>

#include "velodyne_driver/driver.h"

namespace velodyne_driver {

VelodyneDriver::VelodyneDriver(const velodyne::VelodyneDriverConf &conf) {
  config_.model = conf.model();

  double packet_rate; // packet frequency (Hz)
  std::string model_full_name;
  if ((config_.model == "64E_S2") ||
      (config_.model == "64E_S2.1")) // generates 1333312 points per second
  {                                  // 1 packet holds 384 points
    packet_rate = 3472.17;           // 1333312 / 384
    model_full_name = std::string("HDL-") + config_.model;
  } else if (config_.model == "64E") {
    packet_rate = 2600.0;
    model_full_name = std::string("HDL-") + config_.model;
  } else if (config_.model ==
             "64E_S3")     // generates 2222220 points per second (half for
                           // strongest and half for lastest)
  {                        // 1 packet holds 384 points
    packet_rate = 5787.03; // 2222220 / 384
    model_full_name = std::string("HDL-") + config_.model;
  } else if (config_.model == "32E") {
    packet_rate = 1808.0;
    model_full_name = std::string("HDL-") + config_.model;
  } else if (config_.model == "32C") {
    packet_rate = 1507.0;
    model_full_name = std::string("VLP-") + config_.model;
  } else if (config_.model == "VLP16") {
    packet_rate = 754; // 754 Packets/Second for Last or Strongest mode 1508 for
                       // dual (VLP-16 User Manual)
    model_full_name = "VLP-16";
  } else {
    std::cerr << "unknown Velodyne LIDAR model: " << config_.model << std::endl;
    packet_rate = 2600.0;
  }
  std::string deviceName(std::string("Velodyne ") + model_full_name);

  config_.rpm = 600.0;
  if (conf.has_rpm()) {
    config_.rpm = conf.rpm();
  }
  std::cout << deviceName << " rotating at " << config_.rpm << " RPM"
            << std::endl;
  double frequency = (config_.rpm / 60.0); // expected Hz rate

  // default number of packets for each scan is a single revolution
  // (fractions rounded up)
  config_.npackets = (int)ceil(packet_rate / frequency);
  if (conf.has_npackets()) {
    config_.npackets = conf.npackets();
  }
  std::cout << "publishing " << config_.npackets << " packets per scan"
            << std::endl;

  // if we are timestamping based on the first or last packet in the scan
  config_.timestamp_first_packet = false;
  if (conf.has_timestamp_first_packet()) {
    config_.timestamp_first_packet = conf.timestamp_first_packet();
  }
  if (config_.timestamp_first_packet)
    std::cout << "Setting velodyne scan start time to timestamp of first packet"
              << std::endl;

  std::string dump_file;
  if (conf.has_pcap()) {
    dump_file = conf.pcap();
  }

  double cut_angle = -0.01;
  if (conf.has_cut_angle()) {
    cut_angle = conf.cut_angle();
  }
  if (cut_angle < 0.0) {
    std::cout << "Cut at specific angle feature deactivated." << std::endl;
  } else if (cut_angle < (2 * M_PI)) {
    std::cout << "Cut at specific angle feature activated. "
                 "Cutting velodyne points always at "
              << cut_angle << " rad." << std::endl;
  } else {
    std::cerr << "cut_angle parameter is out of range. Allowed range is "
              << "between 0.0 and 2*PI or negative values to deactivate "
                 "this feature."
              << std::endl;
    cut_angle = -0.01;
  }

  // Convert cut_angle from radian to one-hundredth degree,
  // which is used in velodyne packets
  config_.cut_angle = int((cut_angle * 360 / (2 * M_PI)) * 100);

  int udp_port = DATA_PORT_NUMBER;
  if (conf.has_port()) {
    udp_port = conf.port();
  }

  config_.enabled = true;
  config_.time_offset = 0;

  // open Velodyne input device or file
  if (dump_file != "") // have PCAP file?
  {
    // read data from packet capture file
    input_.reset(
        new velodyne_driver::InputPCAP(conf, udp_port, packet_rate, dump_file));
  } else {
    // read data from live socket
    input_.reset(new velodyne_driver::InputSocket(conf, udp_port));
  }

  last_azimuth_ = -1;
}

/** poll the device
 *
 *  @returns true unless end of file reached
 */
bool VelodyneDriver::poll(velodyne::VelodyneScan *scan) {
  if (!config_.enabled) {
    // If we are not enabled exit once a second to let the caller handle
    // anything it might need to, such as if it needs to exit.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
  }

  // Allocate a new shared pointer for zero-copy sharing with other nodelets.
  scan->Clear();

  if (config_.cut_angle >= 0) // Cut at specific angle feature enabled
  {
    scan->mutable_packets()->Reserve(config_.npackets);
    velodyne::VelodynePacket tmp_packet;
    while (true) {
      while (true) {
        int rc = input_->getPacket(&tmp_packet, config_.time_offset);
        if (rc == 0)
          break; // got a full packet?
        if (rc < 0)
          return false; // end of file reached?
      }
      scan->add_packets()->CopyFrom(tmp_packet);

      // Extract base rotation of first block in packet
      std::size_t azimuth_data_pos = 100 * 0 + 2;
      const char *data = tmp_packet.data().data();
      int azimuth = *((const u_int16_t *)(data + azimuth_data_pos));

      // if first packet in scan, there is no "valid" last_azimuth_
      if (last_azimuth_ == -1) {
        last_azimuth_ = azimuth;
        continue;
      }
      if ((last_azimuth_ < config_.cut_angle && config_.cut_angle <= azimuth) ||
          (config_.cut_angle <= azimuth && azimuth < last_azimuth_) ||
          (azimuth < last_azimuth_ && last_azimuth_ < config_.cut_angle)) {
        last_azimuth_ = azimuth;
        break; // Cut angle passed, one full revolution collected
      }
      last_azimuth_ = azimuth;
    }
  } else // standard behaviour
  {
    // Since the velodyne delivers data at a very high rate, keep
    // reading and publishing scans as fast as possible.
    while (scan->packets_size() < config_.npackets) {
      scan->add_packets();
    }
    for (int i = 0; i < config_.npackets; ++i) {
      while (true) {
        // keep reading until full packet received
        int rc =
            input_->getPacket(scan->mutable_packets(i), config_.time_offset);
        if (rc == 0)
          break; // got a full packet?
        if (rc < 0)
          return false; // end of file reached?
      }
    }
  }

  // publish message using time of last packet read
  // std::cout << "Publishing a full Velodyne scan." << std::endl;
  if (config_.timestamp_first_packet) {
    scan->set_stamp(scan->packets(0).stamp());
  } else {
    scan->set_stamp(scan->packets(scan->packets_size() - 1).stamp());
  }

  return true;
}

} // namespace velodyne_driver
