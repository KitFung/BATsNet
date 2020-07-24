#pragma once

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include "include/common.h"
#include "proto_gen/scheduler.pb.h"

namespace scheduler {
class Mission {
public:
  Mission(const std::string &name, const MissionSetting &setting);
  // virtual ~Mission() { running_ = false; }
  ~Mission() { running_ = false; }
  void Run();

protected:
  // Init -> Start -> Stop -> Destory
  // If is repeated task, loop Start Stop
  virtual bool Init() = 0;
  virtual bool Start() = 0;
  virtual bool Stop() = 0;
  virtual bool Destroy() = 0;

private:
  bool SetupCommunToScheduler();

  bool RegisterMission();
  bool WaitForTheStart();
  bool NotifyMissionDone();

  void Handle(PongPacket *packet, ClientConnection *conn);
  void Handle(RegisterAck *packet, ClientConnection *conn);
  void Handle(AllowStartPacket *packet, ClientConnection *conn);
  void Handle(MissionDoneAck *packet, ClientConnection *conn);

  void OverallFlow();
  void CoreFlow();

  void PingPong();
  void ControlLoop();

  pid_t pid_;
  std::string name_;
  MissionSetting setting_;
  std::mutex start_mutex_;
  std::condition_variable start_cv_;
  std::atomic<bool> can_start_ = {false};

  bool running_ = true;
  int ping_fail_ = 0;
  bool registered_ = false;
  double last_pong_ = 0;

  std::shared_ptr<MissionDoneAck> done_msg_;

  int client_sock_;
  std::thread ping_pong_;
  PingPacket ping_packet_;
  ClientConnection conn_;
  PacketHandleMap handle_map_;
};

} // namespace scheduler