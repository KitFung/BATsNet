#pragma once

namespace common {

template <typename StateT, typename OutT> class ConfigHandler {
public:
  ConfigHandler() {}
  virtual ~ConfigHandler() {}
  virtual OutT UpdateConfig(const StateT &old_state,
                            const StateT &new_state) = 0;
};
} // namespace common
