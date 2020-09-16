#pragma once

#include <zlib.h>

#include <fstream>
#include <string>
#include <vector>

namespace data_collector {

constexpr char kBufFolder[] = "/opt/aiot/buf";
constexpr char kBufTmpFolder[] = "/opt/aiot/bufTmp";

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
  std::string fname_;
  std::string sfname_;
  std::vector<char> buf_;
  gzFile f_;
};

class BufReader {
public:
  BufReader(const std::string &file);
  bool Read(char *buf, int *len, const int buf_size);
  void RemoveBuf();

private:
  std::string fname_;
  gzFile f_;
  std::vector<char> buf_;
};
} // namespace data_collector
