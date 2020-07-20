#include <iostream>

namespace scheduler {
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

} // namespace scheduler
