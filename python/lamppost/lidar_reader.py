import grpc
import lamppost.common_pb2 as common_pb2
import lamppost.lidar_pb2 as lidar_pb2
import lamppost.lidar_pb2_grpc as lidar_pb2_grpc
import lamppost.velodyne_py as velodyne_py
import lamppost.service_discovery_py as service_discovery_py
from lamppost.reader import DataReader


def GetConf(channel):
    channel = grpc.insecure_channel(channel)
    stub = lidar_pb2_grpc.ControllerStub(channel)
    msg = common_pb2.Empty()
    res = stub.GetInitDeviceConf(msg)
    print(res)
    return res


class LidarReader(DataReader):
    def __init__(self, data_identifier):
        control_path = service_discovery_py.get_service_path(
            '/_control' + data_identifier)
        conf = GetConf(control_path)
        self.reader = velodyne_py.VelodyneCloudReader(
            data_identifier, conf.SerializeToString())

    def read(self):
        return self.reader.recv()
