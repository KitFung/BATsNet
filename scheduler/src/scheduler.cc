#include "include/scheduler.h"

#include <assert.h>

namespace scheduler {
Scheduler::Scheduler() {}

void Scheduler::Setup() {
  server_sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_sock_ == -1) {
    perror("Fail in create server sock");
    exit(1);
  }
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, kSchedulerSock, sizeof(kSchedulerSock));
  if (remove(kSchedulerSock) == -1 && errno != ENOENT) {
    perror("Fail to clear the sock file at setup");
    exit(1);
  }
  if (bind(server_sock_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
    perror("Fail to bind the address");
    exit(1);
  }

  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::PING, PingPacket,
                         this->HandlePingPacket);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::REGISTER, RegisterPacket,
                         this->HandleRegisterPacket);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::ALLOW_START_ACK, AllowStartAck,
                         this->HandleAllowStartAck);
  REGISTER_PACKET_HANDLE(handle_map_, CTL_FLAG::MISSION_DONE, MissionDone,
                         this->HandleMissionDone);

  for (int i = 0; i < kMissionWorker; ++i) {
    mission_workers_[i] = std::thread([this, i]() {
      while (running_) {
        {
          std::queue<std::shared_ptr<ClientConnection>> q;
          {
            std::lock_guard<std::mutex> lock(mtxs_[i]);
            q.swap(mission_socks_[i]);
          }

          std::queue<std::shared_ptr<ClientConnection>> still_alive;
          while (!q.empty()) {
            auto conn = q.front();
            q.pop();
            int read_len = recv(conn->sock + conn->buf_len, conn->buf,
                                kConnectionBufSize - conn->buf_len, 0);
            if (read_len == -1) {
              close(conn->sock);
              continue;
            } else if (read_len > 0) {
              handle_map_.HandleIfPossible(conn.get());
            }
            if (IsStillValid(conn.get())) {
              still_alive.emplace(conn);
            }
          }

          {
            std::lock_guard<std::mutex> lock(mtxs_[i]);
            while (!still_alive.empty()) {
              auto conn = still_alive.front();
              still_alive.pop();
              mission_socks_[i].emplace(conn);
            }
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }
  mission_thread_ = std::thread([this]() { ListenToMissions(); });
  schedule_thread_ = std::thread([this]() { TimeLoop(); });
}

void Scheduler::ListenToMissions() {
  while (running_) {
    int wi = 0;
    int sock = accept(server_sock_, nullptr, nullptr);
    if (sock != -1) {
      {
        std::lock_guard<std::mutex> lock(mtxs_[wi]);
        auto conn = std::shared_ptr<ClientConnection>(new ClientConnection);
        conn->sock = sock;
        conn->state = MISSION_STATE::INIT;
        conn->last_ping_recv =
            std::chrono::system_clock::now().time_since_epoch().count();
        mission_socks_[wi].emplace(conn);
      }
      wi = (wi + 1) & kMissionWorker;
    }
  }
}

void Scheduler::HandlePingPacket(PingPacket *packet, ClientConnection *conn) {
  PongPacket packet;
  send(conn->sock, &packet, sizeof(PongPacket), 0);
  conn->last_ping_recv =
      std::chrono::system_clock::now().time_since_epoch().count();
  if (conn->state == MISSION_STATE::NOT_INIT) {
    conn->state = MISSION_STATE::INIT;
  }
}

void Scheduler::HandleRegisterPacket(RegisterPacket *packet,
                                     ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::INIT);
  conn->state = MISSION_STATE::REGISTERED;
}

void Scheduler::HandleAllowStartAck(AllowStartAck *packet,
                                    ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::WAITING_START_CONFIRM);
  conn->state = MISSION_STATE::RUNNING;
}

void Scheduler::HandleMissionDone(MissionDone *packet, ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::RUNNING);
  conn->state = MISSION_STATE::DONE;
}

bool Scheduler::IsStillValid(const ClientConnection *conn) const {
  auto now = std::chrono::system_clock::now().time_since_epoch().count();
  // 5 second
  if (now - conn->last_ping_recv > 5 * 1e9) {
    return false;
  }
  return conn->state != MISSION_STATE::DONE;
}

void Scheduler::RemoveFromScheduling(const ClientConnection *) {}

} // namespace scheduler