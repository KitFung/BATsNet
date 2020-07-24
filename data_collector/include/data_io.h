#pragma once

#include <fstream>
#include <string>

namespace data_collector {

constexpr char kBufFolder[] = "/opt/buf";

class BufFolderLock {
public:
  BufFolderLock(const std::string &folder);
  ~BufFolderLock();

private:
  int fd_ = 0;
};

class BufWriter {
public:
  BufWriter(const std::string &topic);
  ~BufWriter();
  void Write(const char *buf, const int len);

private:
  std::fstream f_;
};

class BufReader {
public:
  BufReader(const std::string &file);
  bool Read(char *buf, int *len, const int buf_size);
  void RemoveBuf();

private:
  std::string fname_;
  std::fstream f_;
};
} // namespace data_collector