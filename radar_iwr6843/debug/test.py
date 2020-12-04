import sys
import os
import grpc
import common_pb2
import radar_pb2 
import radar_pb2_grpc

def run():
    channel = grpc.insecure_channel('localhost:50051')
    stub = radar_pb2_grpc.ControllerStub(channel)
    msg = common_pb2.Empty()
    print("-----------Get Conf----------")
    res = stub.GetConf(msg)
    print(res)
    print("-----------Get State----------")
    state = stub.GetState(msg)
    print(state)
    # print("-----------Turn Off----------")
    # state.base.mode = common_pb2.BasicMutableState.Mode.Value('OFF')
    # res = stub.SetState(state)
    # print(res)
    print("-----------SHARING----------")
    state.base.mode = common_pb2.BasicMutableState.Mode.Value('SHARING')
    res = stub.SetState(state)
    print(res)

    print("-----------LocalSaving----------")
    state.base.mode = common_pb2.BasicMutableState.Mode.Value('LOCAL_SAVING')
    res = stub.SetState(state)
    print(res)

    print("-----------Change Mode----------")
    state.base.mode = common_pb2.BasicMutableState.Mode.Value('SHARING')
    state.current_mode = "OUTDOOR"
    res = stub.SetState(state)
    print(res)

    print("-----------Write Manual Cfg-----------")
    manual_cfg = "sensorStop\nflushCfg\nsensorStart\n"
    cfg_object = radar_pb2.LadarCfg()
    cfg_object.data = manual_cfg
    res = stub.SetManualCfg(cfg_object)
    print(res)

if __name__ == '__main__':
    run()
