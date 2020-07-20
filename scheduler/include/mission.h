#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "include/common.h"
#include "proto_gen/scheduler.pb.h"

namespace scheduler {
class Mission {
public:
  Mission(const std::string &name, const MissionSetting &setting);
  virtual ~Mission() { running_ = false; }
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

  void OverallFlow();
  void CoreFlow();

  void PingPong();
  void ControlLoop();
  void HandleRecv(const char *buf, const int len);

  pid_t pid_;
  std::string name_;
  MissionSetting setting_;

  bool running_ = true;
  int ping_fail_ = 0;
  int client_sock_;
  std::thread ping_pong_;
  PingPacket ping_packet_;
};

} // namespace scheduler