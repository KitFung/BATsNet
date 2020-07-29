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

  debug_thread_ = std::thread([this]() {
    while (running_) {
      std::cout << "Conn Count: " << all_conns_.size()
                << " | Name Count: " << nameset_.size()
                << " | Pending Count: " << pending_mission_.size()
                << " | Waiting Count: " << waiting_mission_.size()
                << " | Running Count: " << running_mission_.size() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
  });
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
    // std::cout << "[Scheduler] New Event: " << nfds << std::endl;
    for (int i = 0; i < nfds; ++i) {
      need_handle_sock_.emplace(static_cast<int>(events[i].data.fd));
    }
    cv_.notify_all();
  }
}

void Scheduler::MissionLoop() {
  std::queue<int> tasks;
  while (running_) {
    {
      std::shared_ptr<ClientConnection> conn;
      {
        std::lock_guard<std::mutex> lock(need_handle_mtx_);
        if (!need_handle_sock_.empty()) {
          std::lock_guard<std::mutex> l2(all_conns_mtx_);
          conn = all_conns_[need_handle_sock_.front()];
          need_handle_sock_.pop();
        }
      }
      if (!conn) {
        std::unique_lock<std::mutex> lk(need_handle_mtx_);
        cv_.wait_for(lk,
                     std::chrono::milliseconds(setting_.schedule_freq_ms()));
        continue;
      }
      std::lock_guard<std::mutex> lock(conn->mtx);
      if (conn->invalid) {
        continue;
      }

      int read_len = recv(conn->sock + conn->buf_len, conn->buf,
                          kConnectionBufSize - conn->buf_len, 0);
      if (read_len > 0) {
        conn->buf_len += read_len;
      }
      // std::cout << "conn->buf_len: " << conn->buf_len << std::endl;
      while (conn->buf_len && handle_map_.HandleIfPossible(conn.get()))
        ;
      if (!IsStillValid(conn.get())) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn->sock, nullptr);
        conn->invalid = true;
        {
          std::lock_guard<std::mutex> l1(all_conns_mtx_);
          all_conns_.erase(conn->sock);
        }
        RemoveFromNameSet(conn->name);
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
  send(conn->sock, &respond_pkt, sizeof(PongPacket), MSG_NOSIGNAL);
}

void Scheduler::HandleRegisterPacket(RegisterPacket *packet,
                                     ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::INIT);

  MissionSetting setting;
  setting.ParseFromArray(packet->setting, packet->setting_len);

  RegisterAck ack;
  auto name = setting.name();

  bool insert_succes = false;

  auto RejectFn = [&]() {
    ack.success = false;
    send(conn->sock, &ack, sizeof(RegisterAck), MSG_NOSIGNAL);
    conn->state = MISSION_STATE::ABORT;
  };
  // Reject of no name or repeated
  if (name.empty() || !TryAddToNameSet(name)) {
    // Reject
    RejectFn();
    return;
  }

  // Accept
  // Construct mission
  conn->name = name;
  ScheduledMission mission;
  mission.name = name;
  mission.priority = setting.priority();
  mission.start_s_in_day = 0;
  if (setting.has_schedule_time()) {
    const auto &t = setting.schedule_time();
    mission.start_s_in_day = t.second() + t.minute() * 60 + t.hour() * 60 * 60;
  }
  mission.real_time = setting.real_time();
  mission.setting.CopyFrom(setting);
  {
    std::lock_guard<std::mutex> l1(all_conns_mtx_);
    mission.conn = all_conns_[conn->sock];
  }
  AddPendingMission(std::move(mission));

  // Response
  ack.success = true;
  send(conn->sock, &ack, sizeof(RegisterAck), MSG_NOSIGNAL);
  conn->state = MISSION_STATE::REGISTERED;
  std::cout << "[Scheduler] Registered " << name << std::endl;
}

void Scheduler::HandleAllowStartAck(AllowStartAck *packet,
                                    ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::WAITING_START_CONFIRM);
  conn->state = MISSION_STATE::RUNNING;
}

void Scheduler::HandleMissionDone(MissionDone *packet, ClientConnection *conn) {
  assert(conn->state == MISSION_STATE::RUNNING);

  MissionDoneAck ack;
  std::cout << "[Scheduler] Receive Mission Done " << conn->name << std::endl;
  conn->state = MISSION_STATE::DONE;
  {
    std::lock_guard<std::mutex> lock(mission_lock_);
    if (running_mission_.find(conn->name) != running_mission_.end()) {
      ack.could_exit = !running_mission_[conn->name].setting.repeated_mission();
    } else {
      ack.could_exit = false;
    }
  }
  send(conn->sock, &ack, sizeof(MissionDoneAck), MSG_NOSIGNAL);
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

void Scheduler::AddPendingMission(ScheduledMission &&mission) {
  std::lock_guard<std::mutex> l1(pending_mission_mtx_);
  pending_mission_.emplace(mission);
}

bool Scheduler::TryAddToNameSet(const std::string &name) {
  std::lock_guard<std::mutex> l2(nameset_mtx_);
  if (nameset_.find(name) == nameset_.end()) {
    nameset_.insert(name);
  }
  return true;
}
void Scheduler::RemoveFromNameSet(const std::string &name) {
  std::lock_guard<std::mutex> l2(nameset_mtx_);
  nameset_.erase(name);
}

void Scheduler::TimeLoop() {
  while (running_) {
    // Update Pending
    auto now_s = CurSecondInToday();
    {
      std::lock_guard<std::mutex> l1(pending_mission_mtx_);
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
    }
    // Update Waiting
    TrySelectWaitingMission();

    // Update Running
    std::lock_guard<std::mutex> lock(mission_lock_);
    std::vector<std::string> remove_mission;
    for (const auto &itr : running_mission_) {
      auto &k = itr.first;
      auto &m = itr.second;
      if (m.conn->state == MISSION_STATE::DONE) {
        remove_mission.emplace_back(k);
        ReleaseMissionResource(m);
      }
    }

    for (const auto &k : remove_mission) {
      auto m = running_mission_[k];
      if (m.setting.repeated_mission()) {
        if (m.setting.has_repeated_mission()) {
          if (m.setting.has_repeated_interval_sec()) {
            m.start_s_in_day = now_s + m.setting.repeated_interval_sec();
          } else {
            m.start_s_in_day = 0;
          }
        }
        AddPendingMission(std::move(m));
        std::cout << "[Scheduler] Reschedule " << k << std::endl;
      } else {
        RemoveFromNameSet(k);
        std::cout << "[Scheduler] Erase " << k << std::endl;
      }
      running_mission_.erase(k);
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
  std::lock_guard<std::mutex> l1(mission_lock_);
  std::lock_guard<std::mutex> l2(mission.conn->mtx);
  resource_tbl_.avail_core -= mission.setting.requirement().cpu_core();
  resource_tbl_.avail_mem -= mission.setting.requirement().memory();
  mission.conn->state = MISSION_STATE::WAITING_START_CONFIRM;
  running_mission_[mission.name] = mission;
  std::cout << "[Scheduler] Add Running Mission " << mission.name << std::endl;

  AllowStartPacket packet;
  send(mission.conn->sock, &packet, sizeof(AllowStartPacket), MSG_NOSIGNAL);
}

void Scheduler::ReleaseMissionResource(const ScheduledMission &mission) {
  resource_tbl_.avail_core += mission.setting.requirement().cpu_core();
  resource_tbl_.avail_mem += mission.setting.requirement().memory();
  // resource_tbl_.avail_gpu += mission.setting.requirement().gpu_core();
}

} // namespace scheduler