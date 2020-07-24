#include "include/data_upload_mission.h"

#include <unistd.h>

#include <chrono>
#include <thread>

#include "include/data_io.h"

namespace fs = std::experimental::filesystem;

namespace data_collector {

DataUploadMission::DataUploadMission(const std::string &name,
                                     const scheduler::MissionSetting &setting,
                                     const char *host, const int port)
    : scheduler::Mission(name, setting) {
  DataCollectParams params;
  params.mode = CollectMode::REAL_TIME_UPLOAD;
  collector_.reset(new DataCollector(host, port));
}

bool DataUploadMission::Init() {
  auto sub_folders_pair = GetAllSubFolder();
  for (const auto &p : sub_folders_pair) {
    const auto &topic = p.first;
    auto files = ReadFileListInSubFolder(p.second);
    topic_files_map_[topic] = files;
  }
  return true;
}

bool DataUploadMission::Start() {
  constexpr int size = 2048;
  char buf[size];
  int len;

  for (const auto &itr : topic_files_map_) {
    const auto &topic = itr.first;
    for (const auto &f : itr.second) {
      BufReader reader(f);
      while (reader.Read(buf, &len, size)) {
        if (!collector_->SendData(topic.c_str(), buf, len)) {
          std::cerr << "One frame data have problem" << std::endl;
        }
      }
      reader.RemoveBuf();
    }
  }

  return true;
}
bool DataUploadMission::Stop() { return true; }
bool DataUploadMission::Destroy() { return true; }

std::vector<std::pair<std::string, std::string>>
DataUploadMission::GetAllSubFolder() const {
  std::vector<std::pair<std::string, std::string>> res;
  if (!fs::exists(kBufFolder)) {
    return res;
  }
  for (const auto &p : fs::directory_iterator(kBufFolder)) {
    auto path = fs::path(p);
    if (fs::is_directory(path)) {
      std::string topic_str = path.filename();
      for (auto &w : topic_str) {
        if (w == '#') {
          w = '/';
        }
      }
      res.emplace_back(topic_str, fs::absolute(p));
    }
  }
  return res;
}

std::vector<std::string> DataUploadMission::ReadFileListInSubFolder(
    const std::string &sub_folder) const {
  std::vector<std::string> files;
  if (!fs::exists(sub_folder)) {
    return files;
  }
  // BufFolderLock lock(sub_folder);

  for (const auto &p : fs::directory_iterator(sub_folder)) {
    auto path = fs::path(p);
    if (fs::is_regular_file(path)) {
      files.emplace_back(fs::absolute(p));
    }
  }
  return files;
};

} // namespace data_collector