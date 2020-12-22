#include "include/service_node.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace service_discovery {

ServiceNode::ServiceNode(const std::string &identifier, const int remote_port,
                         const int local_port)
    : identifier_(identifier), service_port_(remote_port),
      local_port_(local_port) {
  etcd_ = std::make_shared<etcd::Client>(ketcd_src);
  local_etcd_ = std::make_shared<etcd::Client>(klocal_etcd_src);
  std::cout << "[ServiceNode] Connected to etcd: " << ketcd_src << std::endl;
  if (service_port_ == 0) {
    val_ = RetreiveServiceIP() + ":" + std::to_string(RetreiveEnvPort());
  } else {
    // Assume the bats side already setu port forward to this specific port
    val_ = RetreiveServiceIP() + ":" + std::to_string(service_port_);
  }
  // Avoid the restart too fast and let register fail
  std::this_thread::sleep_for(std::chrono::seconds(loop_interval_s_ * 4));
  inner_loop_ = std::thread([&] { InnerLoop(); });
  // Just a interval to it register
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

// For register video stream
ServiceNode::ServiceNode(const std::string &identifier, const std::string &path)
    : identifier_(identifier) {
  etcd_ = std::make_shared<etcd::Client>(ketcd_src);
  local_etcd_ = std::make_shared<etcd::Client>(klocal_etcd_src);

  std::cout << "[ServiceNode] Connected to etcd: " << ketcd_src << std::endl;

  val_ = path;

  // Avoid the restart to fast and let register fail
  std::this_thread::sleep_for(std::chrono::seconds(loop_interval_s_ * 4));
  inner_loop_ = std::thread([&] { InnerLoop(); });
  // Just a interval to it register
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

ServiceNode::~ServiceNode() {
  running_ = false;
  if (inner_loop_.joinable()) {
    inner_loop_.join();
  }
  if (registered_) {
    etcd_->rm(identifier_);
  }
}

std::string ServiceNode::SendUdp(const std::string &msg) const {
  // Simlpe UDP
  int sockfd;
  char buffer[1024];
  const char *c_msg = msg.c_str();
  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }
  memset(&servaddr, 0, sizeof(servaddr));

  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(kBATs_qport);
  servaddr.sin_addr.s_addr = inet_addr(kBATs_ip);
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  int n = 0;
  socklen_t len;

  sendto(sockfd, (const char *)c_msg, msg.size(), MSG_CONFIRM,
         (const struct sockaddr *)&servaddr, sizeof(servaddr));
  n = recvfrom(sockfd, (char *)buffer, sizeof(buffer), MSG_WAITALL,
               (struct sockaddr *)&servaddr, &len);
  if (n == -1) {
    perror("Cannot connect to bats ip query");
    exit(EXIT_FAILURE);
  }

  buffer[n] = '\0';
  close(sockfd);
  return std::string(buffer);
}

std::string ServiceNode::RetreiveServiceIP() const {
  std::string msg("GET_IP");
  return SendUdp(msg);
}

int ServiceNode::RetreiveEnvPort() const {
  if (local_port_ == 0) {
    //   // Don't the remote side setup proxy
    //   if (const char *port = std::getenv("SERVICE_BROKER_PORT")) {
    //     int p = std::atoi(port);
    //     return p;
    //   }

    // the port of MQTT broker
    return 1883;
  } else {
    // Require the remote side setup proxy
    std::string msg("GET_PROXY_PORT|");
    msg += identifier_ + "|" + std::to_string(local_port_);
    std::cout << "[ServiceNode] Send msg: " << msg << std::endl;
    std::string respond = SendUdp(msg);
    return std::stoi(respond);
  }
}

void ServiceNode::Register() {
  auto respond = etcd_->add(identifier_, val_, loop_interval_s_ * 3).get();
  if (local_etcd_) {
    local_etcd_->add(identifier_, val_, loop_interval_s_ * 3);
  }
  std::cout << "[ServiceNode] Registered as: " << identifier_ << " -> "
            << etcd_->get(identifier_).get().value().as_string() << std::endl;

  int err_code = respond.error_code();
  if (err_code != 0) {
    std::cout << "[ServiceNode] err_code: " << err_code << " "
              << respond.error_message() << std::endl;
    if (err_code == 105) {
      std::cerr << "[ServiceNode] Another process have register the same id"
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  registered_ = 1;
}

void ServiceNode::RenewRegister() {
  etcd_->set(identifier_, val_, loop_interval_s_ * 3);
  if (local_etcd_) {
    local_etcd_->set(identifier_, val_, loop_interval_s_ * 3);
  }
}

void ServiceNode::ValidProxyAlive() {
  auto proxy_val = etcd_->get(val_).get().value().as_string();
  if (proxy_val != identifier_) {
    std::cerr
        << "[ServiceNode] Some unexpected issue to the proxy -> proxy_val: "
        << proxy_val << "  identifier_: " << identifier_ << "  val_: " << val_
        << std::endl;
    exit(EXIT_FAILURE);
  }
}

void ServiceNode::InnerLoop() {
  Register();
  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(loop_interval_s_));
    RenewRegister();
    if (local_port_ != 0) {
      ValidProxyAlive();
    }
  }
}

} // namespace service_discovery
