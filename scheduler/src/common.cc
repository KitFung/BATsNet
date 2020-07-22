#include "include/common.h"

namespace scheduler {
void PacketHandleMap::RegisterHandle(
    const CTL_FLAG flag,
    std::function<bool(char *, int *, const int, ClientConnection *)> cb) {
  handles_[flag] = cb;
}
bool PacketHandleMap::HandleIfPossible(char *buf, int *buf_len,
                                       const int buf_size,
                                       ClientConnection *conn) {
  if (*buf_len >= sizeof(CTL_FLAG)) {
    CTL_FLAG flag;
    memcpy(&flag, buf, sizeof(CTL_FLAG));
    if (handles_.find(flag) != handles_.end()) {
      return handles_[flag](buf, buf_len, buf_size, conn);
    }
  }
  return false;
}

} // namespace scheduler