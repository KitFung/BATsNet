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

ServiceNode::ServiceNode(const std::string &identifier, const int port)
    : identifier_(identifier), cport_(port) {
  etcd_ = std::make_shared<etcd::Client>(ketcd_src);
  std::cout << "Connected to etcd: " << ketcd_src << std::endl;
  if (cport_ == 0) {
    val_ = RetreiveBrokerIP() + ":" + std::to_string(RetreiveEnvPort());
  } else {
    val_ = RetreiveBrokerIP() + ":" + std::to_string(cport_);
  }
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

std::string ServiceNode::RetreiveBrokerIP() const {
  // Simlpe UDP
  int sockfd;
  char buffer[1024];
  const char *msg = "GET_IP";
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

  sendto(sockfd, (const char *)msg, strlen(msg), MSG_CONFIRM,
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

int ServiceNode::RetreiveEnvPort() const {
  if (const char *port = std::getenv("SERVICE_BROKER_PORT")) {
    int p = std::atoi(port);
    return p;
  }
  return 1883;
}

void ServiceNode::Register() {
  auto respond = etcd_->add(identifier_, val_, loop_interval_s_ * 3).get();
  std::cout << "Registered as: " << identifier_ << " -> "
            << etcd_->get(identifier_).get().value().as_string() << std::endl;

  int err_code = respond.error_code();
  if (err_code != 0) {
    std::cout << "err_code: " << err_code << " " << respond.error_message()
              << std::endl;
    if (err_code == 105) {
      perror("Another process have register the same id");
      exit(EXIT_FAILURE);
    }
  }
  registered_ = 1;
}

void ServiceNode::RenewRegister() {
  etcd_->set(identifier_, val_, loop_interval_s_ * 3);
}

void ServiceNode::InnerLoop() {
  Register();
  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(loop_interval_s_));
    RenewRegister();
  }
}

} // namespace service_discovery
