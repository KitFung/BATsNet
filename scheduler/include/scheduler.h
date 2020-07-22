#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

#include "include/common.h"
namespace scheduler {
constexpr int kMissionWorker = 2;

class Scheduler {
public:
  Scheduler();
  void Run();

private:
  // Interactive to Mission
  void Setup();
  void ListenToMissions();
  void HandlePingPacket(PingPacket *packet, ClientConnection *conn);
  void HandleRegisterPacket(RegisterPacket *packet, ClientConnection *conn);
  void HandleAllowStartAck(AllowStartAck *packet, ClientConnection *conn);
  void HandleMissionDone(MissionDone *packet, ClientConnection *conn);

  // Maintain after interact to Mission
  bool IsStillValid(const ClientConnection *conn) const;
  void RemoveFromScheduling(const ClientConnection *);

  // The scheduling
  void TimeLoop();
  void SelectMission();

  bool running_ = true;
  int server_sock_;

  std::thread mission_thread_;
  std::array<std::thread, kMissionWorker> mission_workers_;
  std::array<std::queue<std::shared_ptr<ClientConnection>>, kMissionWorker>
      mission_socks_;
  std::array<std::mutex, kMissionWorker> mtxs_;
  PacketHandleMap handle_map_;

  std::thread schedule_thread_;
};

} // namespace scheduler