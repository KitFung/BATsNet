#include "include/radar_control.h"

#include <fstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "include/iwr6843_control.h"

namespace radar {

RadarControl::RadarControl(const DeviceConf &device_conf)
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

  deamon_ = std::make_unique<CameraDeamon>(conf_.base().service_cmd(), argv);

  BuildHandler(conf_.model());
  WriteNewDeviceConf();

  std::lock_guard<std::mutex> lock(deamon_->GetLock());
  deamon_->StartDevice();
}

Status RadarControl::GetConf(ServerContext *context,
                             const common::Empty *request,
                             ControllerConf *reply) {
  reply->CopyFrom(conf_);
  return Status::OK;
}
Status RadarControl::GetState(ServerContext *context,
                              const common::Empty *request,
                              ControllerMutableState *reply) {

  std::lock_guard<std::mutex> lock(state_mtx_);
  reply->CopyFrom(state_);
  return Status::OK;
}

Status RadarControl::SetState(ServerContext *context,
                              const ControllerMutableState *request,
                              SetStateResponse *reply) {
  if (!state_mtx_.try_lock()) {
    return Status(grpc::StatusCode::UNAVAILABLE,
                  "State of device is updating. Please wait a minutes");
  }
  ControllerMutableState backup = state_;
  if (!HandleHewConf(*request, reply)) {
    // Try roll back
    SetStateResponse t;
    HandleHewConf(backup, &t);
    state_mtx_.unlock();
    return Status(grpc::StatusCode::UNKNOWN,
                  "Unknown Error while handling the request");
  } else {
    state_mtx_.unlock();
    return Status::OK;
  }
}

void RadarControl::BuildHandler(const Model &model) {
  switch (model) {
  case Model::IWR6843:
    conf_handler_.reset(new IWR68943ConfigHandler());
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }
}

bool RadarControl::HandleHewConf(const ControllerMutableState &new_state,
                                 SetStateResponse *reply) {
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

void RadarControl::WriteNewDeviceConf() {
  device_conf_.mutable_state()->CopyFrom(state_);
  std::fstream fw(conf_.base().device_config_path(),
                  std::ios::out | std::ios::binary);
  google::protobuf::io::OstreamOutputStream output(&fw);
  google::protobuf::TextFormat::Print(device_conf_, &output);
}

} // namespace radar
