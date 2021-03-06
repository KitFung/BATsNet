#pragma once

#include <fstream>
#include <memory>
#include <mutex>

#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>

#include "device/include/config_handler.hpp"
#include "device/include/device_deamon.hpp"

using grpc::ServerContext;
using grpc::Status;

namespace device {

const char kMetaKeyTaskSecret[] = "task-secret-bin";
const char kAclEnvKey[] = "ACL_SERVER";
const char kAclServer[] = "http://137.189.97.26:30777";

#define ACL_VERIFICATION(ctx)                                                  \
  if (!AclVerify(ctx)) {                                                       \
    return Status(grpc::StatusCode::PERMISSION_DENIED,                         \
                  "The task secret in metadata cannot pass the acl");          \
  }

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
    ACL_VERIFICATION(context)

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
  // Require a deamon_ lock before call this
  void RestartDevice() {
    deamon_->StopDevice();
    WriteNewDeviceConf();
    deamon_->StartDevice();
  }
  std::unique_ptr<ConfigHandler> &ConfHandler() { return conf_handler_; }
  virtual void BuildHandler() {
    std::cout << "Called base handler" << std::endl;
  }
  bool AclVerify(const ServerContext *context) const {
    const auto &meta_dat = context->client_metadata();
    auto task_secret_itr = meta_dat.equal_range(kMetaKeyTaskSecret);
    for (auto itr = task_secret_itr.first; itr != task_secret_itr.second;
         ++itr) {
      common::TaskSecret secret;
      secret.ParseFromArray(itr->second.data(), itr->second.size());
      std::cout << "===================== META:" << std::endl;
      std::cout << secret.DebugString() << std::endl;

      std::string acl_server_url(kAclServer);
      if (const char *env_acl_server = std::getenv(kAclEnvKey)) {
        acl_server_url = env_acl_server;
      }

      web::json::value obj;
      // Use identifier instead of controller_identifier is better
      // Since the user may not know the controller_identifier.
      obj[U("ServiceName")] =
          web::json::value::string(U(conf_.base().identifier()));
      obj[U("PodName")] = web::json::value::string(U(secret.task_name()));
      obj[U("PodID")] = web::json::value::string(U(secret.task_id()));

      web::http::client::http_client client(acl_server_url);
      auto resp = client.request(web::http::methods::POST, U("/"), obj);
      if (resp.get().status_code() == web::http::status_codes::Accepted) {
        return true;
      }
    }
    return false;
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

    *reply = conf_handler_->UpdateConfig(state_, new_state);
    state_.CopyFrom(reply->last_state());

    RestartDevice();
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
