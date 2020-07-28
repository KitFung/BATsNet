#include "include/scheduler.h"

#include <assert.h>

namespace scheduler {

constexpr int kMaxSecondInDay = 24 * 60 * 60;

Scheduler::Scheduler(const SchedulerSetting &setting) : setting_(setting) {
  resource_tbl_.total_core = resource_tbl_.avail_core =
      std::thread::hardware_concurrency();
  uint64_t mem_mb = 0;
  long pages = sysconf(_SC_PHYS_PAGES);
  // unit: bytes
  long page_size = sysconf(_SC_PAGE_SIZE);
  mem_mb = (pages * page_size) >> 20;
  resource_tbl_.total_mem = resource_tbl_.avail_mem = mem_mb;
  // @TODO(someone) Add gpu support
  resource_tbl_.total_gpu = resource_tbl_.avail_gpu = 0;
}
Scheduler::~Scheduler() {
  close(epoll_fd_);
  close(server_sock_);
}

void Scheduler::Setup() {
  std::cout << "[Scheduler] Setup" << std::endl;
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
  if (listen(server_sock_, 100) < 0) {
    perror("Fail to Listen");
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

  epoll_fd_ = epoll_create(kMaxEvents);
  accept_thread_ = std::thread([this]() { AcceptConnections(); });
  mission_thread_ = std::thread([this]() { ListenToMissions(); });
  for (int i = 0; i < kMissionWorker; ++i) {
    mission_workers_[i] = std::thread([this]() { MissionLoop(); });
  }
  // schedule_thread_ = std::thread([this]() { TimeLoop(); });
  std::cout << "[Scheduler] Listening to : " << kSchedulerSock << std::endl;
}

void Scheduler::Run() { TimeLoop(); }

void Scheduler::AcceptConnections() {
  while (running_) {
    int sock = accept(server_sock_, nullptr, nullptr);
    if (sock != -1) {
      {
        auto conn = std::shared_ptr<ClientConnection>(new ClientConnection);
        conn->sock = sock;
        conn->state = MISSION_STATE::NOT_INIT;
        conn->last_ping_recv =
            std::chrono::system_clock::now().time_since_epoch().count();
        {
          std::lock_guard<std::mutex> l1(all_conns_mtx_);
          all_conns_[conn->sock] = conn;
        }
        epoll_event ev;
        ev.data.fd = sock;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock, &ev);

        std::cout << "[Scheduler] New Mission" << std::endl;
      }
    }
  }
}

void Scheduler::ListenToMissions() {
  epoll_event events[kMaxEvents];
  while (running_) {
    int nfds = epoll_wait(epoll_fd_, events, kMaxEvents, 5000);
    std::lock_guard<std::mutex> l1(need_handle_mtx_);
    std::lock_guard<std::mutex> l2(all_conns_mtx_);
    std::cout << "[Scheduler] New Event: " << nfds << std::endl;
    for (int i = 0; i < nfds; ++i) {
      need_handle_sock_.emplace(all_conns_[events[i].data.fd]);
    }
  }
}

void Scheduler::MissionLoop() {
  std::queue<int> tasks;
  while (running_) {
    {
      std::shared_ptr<ClientConnection> conn;
      if (!need_handle_sock_.empty()) {
        std::lock_guard<std::mutex> lock(need_handle_mtx_);
        conn = need_handle_sock_.front();
        need_handle_sock_.pop();
      }
      if (!conn) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(setting_.schedule_freq_ms()));
        continue;
      }
      std::cout << "Handle a Mission Recv" << std::endl;
      int read_len = recv(conn->sock + conn->buf_len, conn->buf,
                          kConnectionBufSize - conn->buf_len, 0);
      if (read_len > 0) {
        conn->buf_len += read_len;
      }
      while (conn->buf_len && handle_map_.HandleIfPossible(conn.get()))
        ;
      if (!IsStillValid(conn.get())) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn->sock, nullptr);
        std::lock_guard<std::mutex> l1(all_conns_mtx_);
        std::lock_guard<std::mutex> l2(conn_map_mtx_);
        all_conns_.erase(conn->sock);
        if (!conn->name.empty()) {
          conn_map_.erase(conn->name);
        }
        std::cout << "[Scheduler] Remove Invalid " << conn->sock << " "
                  << conn->name << std::endl;
      }
    }
  }
}

void Scheduler::HandlePingPacket(PingPacket *packet, ClientConnection *conn) {
  if (conn->state == MISSION_STATE::NOT_INIT) {
    conn->state = MISSION_STATE::INIT;
  }
  conn->last_ping_recv =
      std::chrono::system_clock::now().time_since_epoch().count();

  PongPacket respond_pkt;
  send(conn->sock, &respond_pkt, sizeof(PongPacket), 0);
}

void Scheduler::HandleRegisterPacket(RegisterPacket *packet,
                                     ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::INIT);

  MissionSetting setting;
  setting.ParseFromArray(packet->setting, packet->setting_len);

  RegisterAck ack;
  auto name = setting.name();
  if (name.empty() || conn_map_.find(name) != conn_map_.end()) {
    // Reject
    ack.success = false;
    send(conn->sock, &ack, sizeof(RegisterAck), 0);
    conn->state = MISSION_STATE::ABORT;
  } else {
    // Accept
    {
      std::lock_guard<std::mutex> l1(all_conns_mtx_);
      std::lock_guard<std::mutex> l2(conn_map_mtx_);
      conn_map_[name] = all_conns_[conn->sock];
    }

    conn->name = name;

    ScheduledMission mission;
    mission.name = name;
    mission.priority = setting.priority();
    mission.start_s_in_day = 0;
    if (setting.has_schedule_time()) {
      const auto &t = setting.schedule_time();
      mission.start_s_in_day =
          t.second() + t.minute() * 60 + t.hour() * 60 * 60;
    }
    mission.real_time = setting.real_time();
    mission.setting.CopyFrom(setting);
    pending_mission_.emplace(mission);

    // Response
    ack.success = true;
    send(conn->sock, &ack, sizeof(RegisterAck), 0);
    conn->state = MISSION_STATE::REGISTERED;
    std::cout << "[Scheduler] Registered " << name << std::endl;
  }
}

void Scheduler::HandleAllowStartAck(AllowStartAck *packet,
                                    ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::WAITING_START_CONFIRM);
  conn->state = MISSION_STATE::RUNNING;
}

void Scheduler::HandleMissionDone(MissionDone *packet, ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::RUNNING);
  std::lock_guard<std::mutex> lock(mission_lock_);

  MissionDoneAck ack;
  conn->state = MISSION_STATE::DONE;
  ack.could_exit = !running_mission_[conn->name].setting.repeated_mission();
  send(conn->sock, &ack, sizeof(MissionDoneAck), 0);
  std::cout << "[Scheduler] Running " << conn->name << std::endl;
}

bool Scheduler::IsStillValid(const ClientConnection *conn) const {
  auto now = std::chrono::system_clock::now().time_since_epoch().count();
  // 5 second
  if (now - conn->last_ping_recv > 5 * 1e9) {
    return false;
  }
  return conn->state != MISSION_STATE::ABORT;
}

int CurSecondInToday() {
  std::time_t t = std::time(0);
  std::tm *now = std::localtime(&t);
  int h = now->tm_hour;
  int m = now->tm_min;
  int s = now->tm_sec;
  return s + m * 60 + h * 60 * 60;
}

void Scheduler::TimeLoop() {
  while (running_) {
    // Update Pending
    auto now_s = CurSecondInToday();
    while (!pending_mission_.empty() &&
           now_s >= pending_mission_.top().start_s_in_day) {
      auto m = pending_mission_.top();
      pending_mission_.pop();
      if (m.real_time) {
        AddRunningMission(m);
      } else {
        waiting_mission_.emplace(m);
      }
    }
    // Update Waiting
    TrySelectWaitingMission();

    // Update Running
    std::lock_guard<std::mutex> lock(mission_lock_);
    std::vector<std::string> remove_mission;
    for (const auto &itr : running_mission_) {
      auto &k = itr.first;
      auto &m = itr.second;
      if (conn_map_[k]->state == MISSION_STATE::DONE) {
        remove_mission.emplace_back(k);
        ReleaseMissionResource(m);
      }
    }

    for (const auto &k : remove_mission) {
      auto m = running_mission_[k];
      running_mission_.erase(k);
      if (m.setting.repeated_mission()) {
        if (m.setting.has_repeated_mission()) {
          if (m.setting.has_repeated_interval_sec()) {
            m.start_s_in_day = now_s + m.setting.repeated_interval_sec();
          } else {
            m.start_s_in_day = 0;
          }
        }
        pending_mission_.emplace(m);
        std::cout << "[Scheduler] Reschedule " << k << std::endl;
      } else {
        conn_map_.erase(k);
        std::cout << "[Scheduler] Erase" << k << std::endl;
      }
    }
    // At mid night, should reduce the start sec of all mission in q
    std::this_thread::sleep_for(
        std::chrono::milliseconds(setting_.schedule_freq_ms()));
  }
}

bool Scheduler::AbleToSelect(const ScheduledMission &mission) const {
  if (mission.setting.has_requirement()) {
    const auto &requir = mission.setting.requirement();
    if (requir.has_cpu_core() && resource_tbl_.avail_core < requir.cpu_core()) {
      return false;
    }
    if (requir.has_memory() && resource_tbl_.avail_mem < requir.memory()) {
      return false;
    }
  }
  return true;
}

void Scheduler::TrySelectWaitingMission() {
  int cur_pri = -1;
  std::vector<ScheduledMission> skipped;
  while (!waiting_mission_.empty()) {
    auto m = waiting_mission_.top();
    waiting_mission_.pop();
    if (AbleToSelect(m)) {
      AddRunningMission(m);
      cur_pri = m.priority;
    } else {
      skipped.emplace_back(m);
      if (cur_pri != -1 && cur_pri != m.priority) {
        break;
      }
    }
  }
  for (const auto &m : skipped) {
    waiting_mission_.emplace(m);
  }
}

void Scheduler::AddRunningMission(const ScheduledMission &mission) {
  std::lock_guard<std::mutex> lock(mission_lock_);
  resource_tbl_.avail_core -= mission.setting.requirement().cpu_core();
  resource_tbl_.avail_mem -= mission.setting.requirement().memory();
  conn_map_[mission.name]->state = MISSION_STATE::WAITING_START_CONFIRM;
  running_mission_[mission.name] = mission;
  AllowStartPacket packet;
  send(conn_map_[mission.name]->sock, &packet, sizeof(AllowStartPacket), 0);
}

void Scheduler::ReleaseMissionResource(const ScheduledMission &mission) {
  resource_tbl_.avail_core += mission.setting.requirement().cpu_core();
  resource_tbl_.avail_mem += mission.setting.requirement().memory();
  // resource_tbl_.avail_gpu += mission.setting.requirement().gpu_core();
}

} // namespace scheduler