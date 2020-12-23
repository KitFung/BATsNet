# Device Controller

Provide the functionality for the device controller.
To create a device controller.

## How to use

### Step 1. First create a grpc proto for the sensor

replace the `sensorXX` to the actual sensor name

```
syntax = "proto2";

import "common.proto";

package sensorXX;

service Controller {
  rpc GetInitDeviceConf(common.Empty) returns (DeviceConf) {}
  rpc GetConf(common.Empty) returns (ControllerConf) {}
  rpc GetState(common.Empty) returns (ControllerMutableState) {}
  rpc SetState(ControllerMutableState) returns (SetStateResponse) {}
}

message SetStateResponse {
  required ControllerMutableState last_state = 1;
}
///////////////////////////////


message ControllerConf {
  required common.BasicConf base = 1;
  # Add any attribute that was suppose to be used by the controller
  # and won't be changed at runtime
}

message ControllerMutableState {
  required common.BasicMutableState base = 1;
  # Add any attribute that was suppose to be used by the controller
  # and may changed at runtime
}

message DeviceConf {
  required ControllerConf conf = 1;
  required ControllerMutableState state = 2;
  # Add any attribute that used by the device driver
}

```

### Step 2. Inherit the headers and instantiate the template

Create the device controller class for the sensor. If you added specific endpoint for the sensor controller grpc, implement and override that function in here.
```
#pragma once

#include "device/include/device_control.hpp"

#include "proto/sensorXX.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

namespace sensorXX {

using BaseControl =
    device::BaseControl<DeviceConf, ControllerConf, ControllerMutableState,
                        SetStateResponse, sensorXX::Controller::Service>;

class SensorXXControl final : public BaseControl {
public:
  SensorXXControl(const DeviceConf &device_conf);

protected:
  void BuildHandler() override;
};

} // namespace sensorXX
```

instantiate the sensor config handler. The handler is used to handle the user request that want to modify the sensor config
```
#pragma once

#include "device/include/config_handler.hpp"
#include "proto/sensorXX.pb.h"

namespace sensorXX {

using ConfigHandler =
    common::ConfigHandler<ControllerMutableState, SetStateResponse>;
} // namespace sensorXX
```

create the ConfigHandler class
```
#pragma once

#include "include/common.h"

namespace sensorXX {

class SensorXXHandler : public ConfigHandler {
public:
  SensorXXHandler();
  ~SensorXXHandler() override;
  SetStateResponse
  UpdateConfig(const ControllerMutableState &old_state,
               const ControllerMutableState &new_state) override;
};

} // namespace sensorXX
```

### Step 3. Implement the device controller


Implement how to handle the user request here.
```
#include "include/sensorxx_handler.h"

namespace sensorXX {

SensorXXHandler::SensorXXHandler() {}
SensorXXHandler::~SensorXXHandler() {}
SetStateResponse
SensorXXHandler::UpdateConfig(const ControllerMutableState &old_state,
                                    const ControllerMutableState &new_state) {
  SetStateResponse response;
  response.mutable_last_state()->CopyFrom(new_state);
  return response;
}

} // namespace sensorXX
```

Implement the actual control class
```
#include "include/sensorXX_control.h"

#include "include/hikvision_control.h"
#include "include/trafi_one_control.h"

namespace sensorXX {

SensorXXControl::SensorXXControl(const DeviceConf &device_conf)
    : BaseControl(device_conf) {
  InitControl();
}

void SensorXXControl::BuildHandler() {
  conf_handler_ = std::make_unique<SensorXXHandler>();
}

} // namespace sensorXX
```

Create the execute binary of the device controller
```
#include <stdlib.h>

#include <iostream>
#include <memory>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "service_discovery/include/service_node.h"

#include "include/sensorXX_control.h"

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "[Usage] " << argv[0] << " pb_conf" << std::endl;
    exit(1);
  }

  camera::DeviceConf conf;
  int fd = open(argv[1], O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &conf)) {
    std::cerr << "Error while parsing conf" << std::endl;
    exit(1);
  }

  const auto &base_conf = conf.conf().base();
  const auto &base_state = conf.state().base();

  std::string server_address = "0.0.0.0:";
  if (base_conf.has_control_service_port()) {
    server_address += base_conf.control_service_port();
  } else {
    server_address += "0";
  }
  sensorXX::SensorXXControl controller(conf);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  int port = 0;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(),
                           &port);
  builder.RegisterService(&controller);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[SensorXXControl] Server listening on "
            << "0.0.0.0:" << port << std::endl;
  service_discovery::ServiceNode node(base_conf.controller_identifier(), 0,
                                      port);
  server->Wait();

  return 0;
}
```
