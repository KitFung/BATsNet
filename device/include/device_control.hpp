#pragma once

#include <fstream>
#include <memory>
#include <mutex>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>

#include "device/include/config_handler.hpp"
#include "device/include/device_deamon.hpp"

using grpc::ServerContext;
using grpc::Status;

namespace device {

template <typename DConfT, typename ConfT, typename StateT,
          typename SetStateResT, typename ServiceT>
class BaseControl : public ServiceT {
public:
  using ConfigHandler = common::ConfigHandler<StateT, SetStateResT>;
  using Deamon = device::DeviceDeamon<
      BaseControl<DConfT, ConfT, StateT, SetStateResT, ServiceT>>;

  BaseControl(const DConfT &device_conf)
      : conf_(device_conf.conf()), state_(device_conf.state()),
        device_conf_(device_conf) {

    std::vector<std::string> argv =
        [](const auto &argv) -> std::vector<std::string> {
      std::vector<std::string> vec;
      for (const auto &str : argv) {
        vec.emplace_back(str);
      }
      return vec;
    }(conf_.base().service_argv());

    deamon_ = std::make_unique<Deamon>(conf_.base().service_cmd(), argv);
  }

  virtual ~BaseControl() {}

  // Must call this function in subclass constructor
  void InitControl() {
    BuildHandler();
    WriteNewDeviceConf();
    std::lock_guard<std::mutex> lock(deamon_->GetLock());
    deamon_->StartDevice();
  }
  Status GetInitDeviceConf(ServerContext *context, const common::Empty *request,
                           DConfT *reply) override {
    reply->CopyFrom(device_conf_);
    return Status::OK;
  }
  // API
  Status GetConf(ServerContext *context, const common::Empty *request,
                 ConfT *reply) override {
    reply->CopyFrom(conf_);
    return Status::OK;
  }

  Status GetState(ServerContext *context, const common::Empty *request,
                  StateT *reply) override {
    std::lock_guard<std::mutex> lock(state_mtx_);
    reply->CopyFrom(state_);
    return Status::OK;
  }

  Status SetState(ServerContext *context, const StateT *request,
                  SetStateResT *reply) override {
    if (!state_mtx_.try_lock()) {
      return Status(grpc::StatusCode::UNAVAILABLE,
                    "State of device is updating. Please wait a minutes");
    }
    auto backup = state_;
    if (!HandleHewConf(*request, reply)) {
      // Try roll back
      SetStateResT t;
      HandleHewConf(backup, &t);
      state_mtx_.unlock();
      return Status(grpc::StatusCode::UNKNOWN,
                    "Unknown Error while handling the request");
    } else {
      state_mtx_.unlock();
      return Status::OK;
    }
  }

protected:
  std::unique_ptr<ConfigHandler> &ConfHandler() { return conf_handler_; }
  virtual void BuildHandler() {
    std::cout << "Called base handler" << std::endl;
  }
  ConfT conf_;
  StateT state_;
  DConfT device_conf_;

private:
  bool HandleHewConf(const StateT &new_state, SetStateResT *reply) {
    if (state_.SerializeAsString() == new_state.SerializeAsString()) {
      // Do nothing
      reply->mutable_last_state()->CopyFrom(state_);
      return true;
    }
    std::cout << "> Set New State ---------------------" << std::endl;
    std::cout << new_state.DebugString() << std::endl;
    if (new_state.base().mode() == common::BasicMutableState::OFF) {
      std::lock_guard<std::mutex> lock(deamon_->GetLock());
      // Don't need to update the conf.
      deamon_->StopDevice();
      state_.CopyFrom(new_state);
      reply->mutable_last_state()->CopyFrom(state_);
      return true;
    }
    std::lock_guard<std::mutex> lock(deamon_->GetLock());

    deamon_->StopDevice();
    *reply = conf_handler_->UpdateConfig(state_, new_state);
    state_.CopyFrom(reply->last_state());

    WriteNewDeviceConf();
    deamon_->StartDevice();

    return true;
  }

  void WriteNewDeviceConf() {
    device_conf_.mutable_state()->CopyFrom(state_);
    std::fstream fw(conf_.base().device_config_path(),
                    std::ios::out | std::ios::binary);
    google::protobuf::io::OstreamOutputStream output(&fw);
    google::protobuf::TextFormat::Print(device_conf_, &output);
  }

  std::unique_ptr<ConfigHandler> conf_handler_;

  bool running_ = true;
  std::mutex state_mtx_;
  std::unique_ptr<Deamon> deamon_;
};

} // namespace device
