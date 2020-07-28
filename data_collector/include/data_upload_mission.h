#pragma once

#include <experimental/filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "include/data_collector.h"
#include "scheduler/include/mission.h"

namespace data_collector {
class DataUploadMission : public scheduler::Mission {
public:
  DataUploadMission(const std::string &name,
                    const scheduler::MissionSetting &setting, const char *host,
                    const int port);
  virtual ~DataUploadMission() {}

private:
  bool Init() override;
  bool Start() override;
  bool Stop() override;
  bool Destroy() override;

  // pair (topic, path)
  std::vector<std::pair<std::string, std::string>> GetAllSubFolder() const;
  std::vector<std::string>
  ReadFileListInSubFolder(const std::string &sub_folder) const;

  std::unordered_map<std::string, std::vector<std::string>> topic_files_map_;
  std::shared_ptr<DataCollector> collector_;
};
} // namespace data_collector