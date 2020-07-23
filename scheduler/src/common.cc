#include "include/common.h"

namespace scheduler {
void PacketHandleMap::RegisterHandle(
    const CTL_FLAG flag, std::function<bool(ClientConnection *)> cb) {
  handles_[flag] = cb;
}
bool PacketHandleMap::HandleIfPossible(ClientConnection *conn) {
  if (conn->buf_len >= sizeof(CTL_FLAG)) {
    CTL_FLAG flag;
    memcpy(&flag, conn->buf, sizeof(CTL_FLAG));
    if (handles_.find(flag) != handles_.end()) {
      return handles_[flag](conn);
    }
  }
  return false;
}

} // namespace scheduler