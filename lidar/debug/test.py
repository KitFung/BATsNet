import sys
import os
import grpc
import common_pb2
import lidar_pb2 
import lidar_pb2_grpc

def run():
    channel = grpc.insecure_channel('10.42.0.100:20000')
    stub = lidar_pb2_grpc.ControllerStub(channel)
    msg = common_pb2.Empty()
    print("-----------Get Conf----------")
    res = stub.GetConf(msg)
    print(res)
    print("-----------Get State----------")
    state = stub.GetState(msg)
    print(state)

if __name__ == '__main__':
    run()
