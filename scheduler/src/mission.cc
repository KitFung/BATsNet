#include "include/mission.h"

#include <errno.h>
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
  return true;
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
      int recvn = recv(client_sock_, recv_buf, recv_buf_size, 0);
      if (recvn == -1) {
        std::cerr << "Lost connection to scheduler" << std::endl;
        running_ = false;
        return;
      }
      HandleRecv(recv_buf, recvn);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });
}

void Mission::HandleRecv(const char *buf, const int len) {
  CTL_FLAG flag;
  memcpy(&flag, buf, sizeof(CTL_FLAG));

  switch (flag) {
  case CTL_FLAG::PONG:
    break;

  case CTL_FLAG::REGISTER_ACK:
    break;

  case CTL_FLAG::ALLOW_START:
    break;

  case CTL_FLAG::MISSION_DONE_ACK:
    break;

  default:
    break;
  }
}

bool Mission::WaitForTheStart() {}

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