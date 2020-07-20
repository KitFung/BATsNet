#include "include/mission.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

namespace scheduler {

const char *kSchedulerSock = "/opt/missions/scheduler.sock";

Mission::Mission(const std::string &name, const MissionSetting &setting)
    : name_(name), setting_(setting) {
  pid_ = getpid();
}

void Mission::Run() { OverallFlow(); }

bool Mission::SetupCommunToScheduler() {
  int client_sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client_sock_ == -1) {
    std::cerr << "[" << setting_.name() << "] Failed to create client sock"
              << std::endl;
    return false;
  }
  fcntl(client_sock_, F_SETFL, O_NONBLOCK);
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, kSchedulerSock, sizeof(addr.sun_path));

  if (connect(client_sock_, reinterpret_cast<sockaddr *>(&addr),
              sizeof(addr)) == -1) {
    std::cerr << "[" << setting_.name() << "] Failed to connect to scheduler"
              << std::endl;
    return false;
  }

  // REGISTER
  if (!RegisterMission()) {
    std::cerr << "[" << setting_.name() << "] Failed to register" << std::endl;
    return false;
  }

  ControlLoop();
}

bool Mission::RegisterMission() {
  RegisterPacket r_pack;
  r_pack.pid = pid_;
  r_pack.name_len = name_.size();
  memcpy(r_pack.name, name_.data(), name_.size());

  auto setting_str = setting_.SerializeAsString();
  r_pack.setting_len = setting_str.size();
  memcpy(r_pack.setting, setting_str.data(), setting_str.size());

  char send_buf[1024];
  memcpy(send_buf, &r_pack, sizeof(r_pack));
  if (send(client_sock_, send_buf, sizeof(r_pack), 0) == -1) {
    std::cerr << "Error while sending setting" << std::endl;
    return false;
  }

  // Need to wait until have success
  int timeout = 10;
  while (timeout-- > 0) {
    if (registered_) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return false;
}

bool Mission::WaitForTheStart() {
  std::unique_lock<std::mutex> lk(start_mutex_);
  while (!start_cv_.wait_for(lk, std::chrono::minutes(10),
                             [&] { return can_start_ == true; })) {
    std::cout << "Still waiting for start" << std::endl;
  }
  can_start_ = false;

  return true;
}

bool Mission::NotifyMissionDone() {
  int max_retry = 3;
  MissionDone packet;
  char buf[sizeof(MissionDone)];
  memcpy(buf, &packet, sizeof(MissionDone));

  send(client_sock_, buf, sizeof(MissionDone), 0);
  while (done_msg_.get() == nullptr) {
    if (!setting_.repeated_mission()) {
      std::cerr << "Fast exit the task " << name_ << std::endl;
      return false;
    }
    max_retry--;
    if (max_retry <= 0) {
      std::cerr << "Force exit the task " << name_ << std::endl;
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  bool keep_going = !(done_msg_->could_exit);
  return keep_going;
}

void Mission::ControlLoop() {
  ping_pong_ = std::thread([&]() {
    constexpr size_t msg_size = sizeof(PingPacket);
    char send_buf[msg_size];
    constexpr size_t recv_buf_size = 2048;
    char recv_buf[recv_buf_size];
    memcpy(send_buf, &ping_packet_, sizeof(PingPacket));
    while (running_) {
      // Ping
      if (send(client_sock_, send_buf, msg_size, 0) == -1) {
        std::cerr << "Error while sending ping" << std::endl;
        ping_fail_++;
      } else {
        ping_fail_ = 0;
      }
      // Handle all the recv
      while (true) {
        int recvn = recv(client_sock_, recv_buf, recv_buf_size, 0);
        if (recvn == -1) {
          std::cerr << "Lost connection to scheduler" << std::endl;
          running_ = false;
          return;
        } else if (recvn == 0) {
          break;
        }
        HandleRecv(recv_buf, recvn);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });
}

void Mission::HandleRecv(const char *buf, const int len) {
  CTL_FLAG flag;
  memcpy(&flag, buf, sizeof(CTL_FLAG));

  switch (flag) {
  case CTL_FLAG::PONG:
    last_pong_ = std::chrono::system_clock::now().time_since_epoch().count();
    break;

  case CTL_FLAG::REGISTER_ACK:
    registered_ = true;
    break;

  case CTL_FLAG::ALLOW_START:
    can_start_ = true;
    start_cv_.notify_one();
    break;

  case CTL_FLAG::MISSION_DONE_ACK: {
    MissionDoneAck ack;
    memcpy(&ack, buf, sizeof(MissionDoneAck));
    done_msg_ = std::make_shared<MissionDoneAck>(ack);
    break;
  }

  default:
    break;
  }
}

void Mission::OverallFlow() {
  if (!SetupCommunToScheduler()) {
    std::cerr << "[" << setting_.name()
              << "] fail to setup connection to scheduler" << std::endl;
    exit(1);
  }
  if (!RegisterMission()) {
    std::cerr << "[" << setting_.name() << "] fail to Register Mission"
              << std::endl;
    exit(1);
  }
  if (!Init()) {
    std::cerr << "[" << setting_.name() << "] fail to Init" << std::endl;
    Destroy();
    NotifyMissionDone();
    exit(1);
  }
  CoreFlow();
  Destroy();
}

void Mission::CoreFlow() {
  do {
    WaitForTheStart();
    if (!Start()) {
      std::cerr << "[" << setting_.name() << "] Problem while Start"
                << std::endl;
    }

    if (!Stop()) {
      std::cerr << "[" << setting_.name() << "] Problem while Start"
                << std::endl;
    }
    // Notify Center and Check whether need next loop
  } while (NotifyMissionDone());
}

} // namespace scheduler