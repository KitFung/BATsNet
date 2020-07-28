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
  conn_.sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn_.sock == -1) {
    std::cerr << "[" << setting_.name() << "] Failed to create client sock"
              << std::endl;
    return false;
  }
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, kSchedulerSock, sizeof(addr.sun_path));

  if (connect(conn_.sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
    std::cerr << "[" << setting_.name()
              << "] Failed to connect to scheduler sock: " << addr.sun_path
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

  std::cout << "[" << setting_.name() << "] setup sock" << std::endl;
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
  if (send(conn_.sock, &r_pack, sizeof(r_pack), 0) == -1) {
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

  AllowStartAck ack;
  send(conn_.sock, &ack, sizeof(AllowStartAck), 0);

  return true;
}

bool Mission::NotifyMissionDone() {
  int max_retry = 3;
  std::cout << "Send Mission Done" << std::endl;
  MissionDone packet;
  send(conn_.sock, &packet, sizeof(MissionDone), 0);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }

  bool keep_going = !(done_msg_->could_exit);
  done_msg_.reset();
  return keep_going;
}

void Mission::ControlLoop() {
  ping_pong_ = std::thread([&]() {
    double send_time = -1;
    while (running_) {
      // Ping
      if (conn_.last_ping_recv > send_time || send_time < 0) {
        send_time = std::chrono::system_clock::now().time_since_epoch().count();
        if (send(conn_.sock, &ping_packet_, sizeof(PingPacket), 0) == -1) {
          std::cerr << "Error while sending ping | " << std::endl;
          ping_fail_++;
        } else {
          ping_fail_ = 0;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
  });
  control_ = std::thread([&]() {
    fd_set set;
    struct timeval st_time;

    st_time.tv_sec = 10;
    st_time.tv_usec = 0;
    FD_ZERO(&set);
    FD_SET(conn_.sock, &set);
    while (running_) {
      int ret = select(conn_.sock + 1, &set, nullptr, nullptr, &st_time);
      if (ret == 0) {
        // printf("select timeout.\n");
      } else if (ret < 0) {
        perror("Select error");
      } else if (ret == 1) {
        if (FD_ISSET(conn_.sock, &set)) {
          int read_len = recv(conn_.sock, conn_.buf + conn_.buf_len,
                              kConnectionBufSize - conn_.buf_len, MSG_DONTWAIT);
          if (read_len > 0) {
            conn_.buf_len += read_len;
            while (handle_map_.HandleIfPossible(&conn_))
              ;
          } else if (read_len == -1) {
            perror("Failed to Read Msg");
            running_ = false;
            return;
          }
        }
      }
    }
  });
  // Make sure the ping is send before the later step
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
  std::cout << "Mission Done Ack" << std::endl;
  done_msg_ = std::make_shared<MissionDoneAck>(*packet);
}

void Mission::OverallFlow() {
  if (!SetupCommunToScheduler()) {
    std::cerr << "[" << setting_.name()
              << "] fail to setup connection to scheduler" << std::endl;
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