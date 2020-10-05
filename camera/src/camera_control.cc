#include "include/camera_control.h"

#include <sys/prctl.h>

#include "include/hikvision_control.h"
#include "include/trafi_one_control.h"

namespace camera {

struct sigaction sa;
CameraControl *global_ptr;

CameraControl::CameraControl(const ControllerConf &conf,
                             const ControllerMutableState &state) {
  conf_ = conf;
  state_ = state;
  const auto model = conf.model();
  switch (model) {
    //   case CameraModel::HIKVISION_21XX:
    //     conf_handler_ = std::make_unique<HikvisionConfigHandler>();
    //     break;
  case CameraModel::TRAFI_ONE_195:
    conf_handler_.reset(new TrafiOneConfigHandler());
    break;
  default:
    exit(EXIT_FAILURE);
    break;
  }

  global_ptr = this;

  StartCamera();
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = Deamon;

  sigaction(SIGCHLD, &sa, NULL);
}

bool CameraControl::HandleHewConf(const ControllerMutableState &new_state) {
  if (state_.SerializeAsString() == new_state.SerializeAsString()) {
    return true;
  } else {
    std::lock_guard<std::mutex> lock(mtx_);
    StopCamera();
    conf_handler_->UpdateConfig(new_state);
    state_ = new_state;
    StartCamera();
  }
  return true;
}

void CameraControl::StartCamera() {
  if (pid_ > 0) {
    return;
  }
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

    const auto cmd = conf_.base().service_cmd();
    const auto vec = conf_.base().service_argv();
    int argc = vec.size();
    const char **argv = new const char *[argc + 2];

    argv[0] = cmd.c_str();

    for (int i = 0; i < argc; ++i) {
      argv[i + 1] = vec[i].c_str();
    }
    argv[argc + 1] = nullptr;
    for (int i = 0; i < argc + 1; ++i) {
      printf("%s\n", argv[i]);
    }
    execv(cmd.c_str(), const_cast<char **>(argv));
    delete[] argv;
    printf("Failed\n");
    exit(EXIT_FAILURE);
  }
}

void CameraControl::StopCamera() {
  if (pid_ == 0) {
    return;
  } else {
    // Kill Process
    kill(pid_, SIGTERM);
    sleep(1);
    kill(pid_, SIGKILL);
    pid_ = 0;
  }
}
void Deamon(int sig) {
  pid_t pid;
  int status;

  if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    std::cout << "Catch Child Process Killed. Restart" << std::endl;
    sleep(5);
    global_ptr->pid_ = 0;
    global_ptr->StartCamera();
  }
}

} // namespace camera