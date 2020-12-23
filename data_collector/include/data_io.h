#pragma once

#include <zlib.h>

#include <fstream>
#include <string>
#include <vector>

namespace data_collector {

constexpr char kBufFolder[] = "/opt/aiot/buf";
constexpr char kBufTmpFolder[] = "/opt/aiot/bufTmp";

/**
 * Lock a folder and avoid multiple operation on the folder
 */
class BufFolderLock {
public:
  BufFolderLock(const std::string &folder);
  ~BufFolderLock();

private:
  int fd_ = 0;
};


/**
 * The buf format: |msg_size: 32 bit|msg : msg_size bit|.....|msg_size: 32 bit|msg : msg_size bit|
 */

/**
 * Write the data in to gz file 
 */
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

/**
 * Read the data from gz file 
 */
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
