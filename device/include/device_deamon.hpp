#pragma once

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "include/common.h"

namespace device {

template <typename ControlClass> class DeviceDeamon {
public:
  DeviceDeamon(const std::string &cmd, const std::vector<std::string> &argv)
      : cmd_(cmd), argv_(argv) {
    if (singleton_ != nullptr) {
      perror("More than one control object is created");
      exit(EXIT_FAILURE);
    }

    should_up_ = false;
    singleton_ = this;

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = DeviceDeamon<ControlClass>::Deamon;

    sigaction(SIGCHLD, &sa, NULL);
  }

  // Don't provide implicit thread-safe for start stop device
  void StartDevice() {
    pid_t ppid_before_fork = getpid();
    pid_ = fork();
    // Child Process
    if (pid_ == 0) {
      int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
      if (r == -1) {
        perror(0);
        exit(1);
      }
      if (getppid() != ppid_before_fork) {
        exit(1);
      }

      int argc = argv_.size();
      const char **argv = new const char *[argc + 2];

      argv[0] = cmd_.c_str();
      for (int i = 0; i < argc; ++i) {
        argv[i + 1] = argv_[i].c_str();
      }
      argv[argc + 1] = nullptr;

      execv(cmd_.c_str(), const_cast<char **>(argv));
      delete[] argv;
      // @TODO(kit) handle the case that cannot start regularrly
      printf("\n\n>>>>>>>>>> Start Device Failed <<<<<<<<<<<<\n\n");
      exit(EXIT_FAILURE);
    } else {
      // printf("pid_ %d\n", pid_);
      should_up_ = true;
    }
  }

  void StopDevice() {
    should_up_ = false;
    if (pid_ == 0) {
      return;
    }
    // Kill Process
    closed_pid_.emplace(pid_);
    kill(pid_, SIGTERM);
    kill(pid_, SIGKILL);
    pid_ = 0;
  }

  std::mutex &GetLock() { return mtx_; }

  // A special singleton. Don't consider thread safe. Assume the obj is created
  // on other places
  static DeviceDeamon<ControlClass> *GetSingleton() {
    if (singleton_ == nullptr) {
      perror("Cannot Get the Control Obj");
      exit(EXIT_FAILURE);
    }
    return singleton_;
  }

  static void Deamon(int sig) {
    pid_t pid;
    int status;

    if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
      std::lock_guard<std::mutex> lock(mtx_);
      if (closed_pid_.find(pid) != closed_pid_.end()) {
        closed_pid_.erase(pid);
        return;
      }
      if (!should_up_) {
        return;
      }
      std::cout << "Catch Child Process Killed. Restart. PID: " << pid
                << std::endl;
      auto obj = GetSingleton();
      obj->pid_ = 0;
      obj->StartDevice();
    }
  }

private:
  std::string cmd_;
  std::vector<std::string> argv_;
  pid_t pid_ = 0;

  static std::mutex mtx_;
  static DeviceDeamon<ControlClass> *singleton_;
  static std::atomic<bool> should_up_;
  static std::unordered_set<int> closed_pid_;
};

template <typename T> DeviceDeamon<T> *DeviceDeamon<T>::singleton_ = nullptr;
template <typename T> std::atomic<bool> DeviceDeamon<T>::should_up_ = {false};
template <typename T> std::mutex DeviceDeamon<T>::mtx_;
template <typename T> std::unordered_set<int> DeviceDeamon<T>::closed_pid_;

} // namespace device
