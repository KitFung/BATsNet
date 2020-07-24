#include "include/mission.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <experimental/filesystem>
#include <iostream>

namespace fs = std::experimental::filesystem;

namespace scheduler {

Mission::Mission(const std::string &name, const MissionSetting &setting)
    : name_(name), setting_(setting) {
  pid_ = getpid();
  fs::create_directories(kSchedulerSockDir);
}

void Mission::Run() {
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::PONG, PongPacket, this->Handle);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::REGISTER_ACK, RegisterAck,
                         this->Handle);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::ALLOW_START, AllowStartPacket,
                         this->Handle);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::MISSION_DONE_ACK,
                         MissionDoneAck, this->Handle);

  OverallFlow();
}

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

  // Setup the loop
  ControlLoop();

  // REGISTER
  if (!RegisterMission()) {
    std::cerr << "[" << setting_.name() << "] Failed to register" << std::endl;
    return false;
  }

  return true;
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
  done_msg_.reset();
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
      int read_len = recv(conn_.sock + conn_.buf_len, conn_.buf,
                          kConnectionBufSize - conn_.buf_len, 0);
      conn_.buf_len += read_len;
      if (read_len > 0) {
        handle_map_.HandleIfPossible(&conn_);
      } else if (read_len == -1) {
        std::cerr << "Lost connection to scheduler" << std::endl;
        running_ = false;
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
}

void Mission::Handle(PongPacket *packet, ClientConnection *conn) {
  conn->last_ping_recv =
      std::chrono::system_clock::now().time_since_epoch().count();
}

void Mission::Handle(RegisterAck *packet, ClientConnection *conn) {
  if (packet->success) {
    registered_ = true;
  } else {
    std::cerr << "Failed to register mission" << std::endl;
    exit(1);
  }
}
void Mission::Handle(AllowStartPacket *packet, ClientConnection *conn) {
  can_start_ = true;
  start_cv_.notify_one();
}

void Mission::Handle(MissionDoneAck *packet, ClientConnection *conn) {
  done_msg_ = std::make_shared<MissionDoneAck>(*packet);
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