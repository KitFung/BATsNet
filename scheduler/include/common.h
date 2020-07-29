#pragma once

#include <string.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
namespace scheduler {
constexpr char kSchedulerSockDir[] = "/opt/aiot/missions";
constexpr char kSchedulerSock[] = "/opt/aiot/missions/scheduler.sock";

enum MISSION_STATE {
  NOT_INIT = 0,
  INIT = 1,
  REGISTERED = 2,
  PENDING = 3,
  WAITING_START_CONFIRM = 4,
  RUNNING = 5,
  DONE = 6,
  ABORT = 7
};

enum class CTL_FLAG {
  // Check alive between scheduler and mission
  PING = 0,
  PONG = 1,

  REGISTER = 2,
  REGISTER_ACK = 3,
  ALLOW_START = 4,
  ALLOW_START_ACK = 5,
  MISSION_DONE = 6,
  MISSION_DONE_ACK = 7,
};

struct PingPacket {
  CTL_FLAG flag = CTL_FLAG::PING;
};

struct PongPacket {
  CTL_FLAG flag = CTL_FLAG::PONG;
};

struct RegisterPacket {
  CTL_FLAG flag = CTL_FLAG::REGISTER;
  uint32_t pid;
  uint32_t name_len;
  char name[128];
  uint32_t setting_len;
  char setting[2048];
};
struct RegisterAck {
  CTL_FLAG flag = CTL_FLAG::REGISTER_ACK;
  bool success;
};

struct AllowStartPacket {
  CTL_FLAG flag = CTL_FLAG::ALLOW_START;
};
struct AllowStartAck {
  CTL_FLAG flag = CTL_FLAG::ALLOW_START_ACK;
};

struct MissionDone {
  CTL_FLAG flag = CTL_FLAG::MISSION_DONE;
};

struct MissionDoneAck {
  CTL_FLAG flag = CTL_FLAG::MISSION_DONE_ACK;
  uint32_t seq;
  bool could_exit;
};

constexpr int kConnectionBufSize = 1 << 16;
struct ClientConnection {
  ~ClientConnection() {
    if (sock > 0) {
      close(sock);
    }
  }
  int sock = 0;
  char buf[kConnectionBufSize];
  int buf_len = 0;

  std::string name;
  MISSION_STATE state = NOT_INIT;
  double last_ping_recv = -1;
  std::mutex mtx;

  bool invalid = false;
};

class PacketHandleMap {
public:
  void RegisterHandle(const CTL_FLAG flag,
                      std::function<bool(ClientConnection *)> cb);
  bool HandleIfPossible(ClientConnection *);

private:
  std::unordered_map<CTL_FLAG, std::function<bool(ClientConnection *)>>
      handles_;
};

#define REGISTER_PACKET_HANDLE(handle_map, flag, PacketType, fn)               \
  handle_map.RegisterHandle(flag, [&](ClientConnection *conn) {                \
    if (conn->buf_len < sizeof(PacketType)) {                                  \
      return false;                                                            \
    }                                                                          \
    PacketType packet;                                                         \
    memcpy(&packet, conn->buf, sizeof(PacketType));                            \
    fn(&packet, conn);                                                         \
    conn->buf_len -= sizeof(PacketType);                                       \
    memmove(conn->buf, conn->buf + sizeof(PacketType), conn->buf_len);         \
    return true;                                                               \
  });

} // namespace scheduler
