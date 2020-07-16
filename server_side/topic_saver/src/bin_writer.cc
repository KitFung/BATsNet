#include "bin_writer.h"

#include <experimental/filesystem>
#include <iostream>

namespace topic_saver {
std::string GetNowTimeStramp() {
  time_t rawtime;
  struct tm *timeinfo;
  char buffer[80];
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timeinfo);
  return std::string(buffer);
}

BinWriter::BinWriter(const char *save_folder) : save_folder_(save_folder) {
  working_thread_ = std::thread([this]() { WorkingLoop(); });
}

BinWriter::~BinWriter() { running_ = false; }
void BinWriter::AddTask(std::shared_ptr<DataBuffer> data) {
  std::lock_guard<std::mutex> lock(mtx_);
  bufs_.emplace(data);
}

void BinWriter::WorkingLoop() {
  while (running_ || !bufs_.empty()) {
    while (!bufs_.empty()) {
      std::shared_ptr<DataBuffer> ptr;
      {
        std::lock_guard<std::mutex> lock(mtx_);
        ptr = bufs_.front();
        bufs_.pop();
      }
      Write(ptr.get());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void BinWriter::Write(const DataBuffer *data) {
  auto fname = GetNowTimeStramp();
  FILE *p_file;
  std::string tpname = data->topic_name;
  for (auto &w : tpname) {
    if (w == '/') {
      w = '#';
    }
  }
  std::string dir = save_folder_ + "/" + tpname;
  std::experimental::filesystem::create_directory(dir);
  std::string save_path = dir + "/" + fname;
  p_file = fopen(save_path.c_str(), "wb");

  uint32_t total_msg = data->message_len;
  fwrite(&total_msg, sizeof(uint32_t), 1, p_file);

  for (uint32_t i = 0; i < total_msg; ++i) {
    // Some calculations to fill a[]
    fwrite(data->each_len + i, sizeof(uint32_t), 1, p_file);
    fwrite(data->buf + i, sizeof(char), data->each_len[i], p_file);
  }
  fclose(p_file);
}

} // namespace topic_saver