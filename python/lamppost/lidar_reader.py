import grpc
import numpy

import lamppost.common_pb2 as common_pb2
import lamppost.lidar_pb2 as lidar_pb2
import lamppost.lidar_pb2_grpc as lidar_pb2_grpc
import lamppost.velodyne_py as velodyne_py
import lamppost.velodyne_pb2 as velodyne_pb2
import lamppost.service_discovery_py as service_discovery_py
from lamppost.reader import DataReader

def GetConf(channel):
    channel = grpc.insecure_channel(channel)
    stub = lidar_pb2_grpc.ControllerStub(channel)
    msg = common_pb2.Empty()
    res = stub.GetInitDeviceConf(msg)
    return res


# The user need to provide it own calibration file
# Or we could provide a api for it to get a recommended calibration file
class LidarReader(DataReader):
    def __init__(self, data_identifier, calibration_file='', max_range=130.0, min_range=0.4):
        # calibration_file = "/home/kit/dev/BATsNet/lidar/lib/velodyne/velodyne_pointcloud/params/VLP16db.yaml"
        if len(calibration_file) == 0:
            raise "Must provide the calibration"
        control_path = service_discovery_py.get_service_path(
            '/_control' + data_identifier)
        dconf = GetConf(control_path)
        conf = velodyne_pb2.VelodynePointCloudConf()
        conf.model = dconf.velodyne_conf.model
        conf.calibration = calibration_file
        conf.max_range = max_range
        conf.min_range = min_range
        conf.scan_topic_name = ''
        conf.cloud_topic_name = ''
        self.reader = velodyne_py.VelodyneCloudReader(
            data_identifier, conf.SerializeToString())

    def read(self):
        return self.reader.recv()
# from lamppost.lidar_reader import LidarReader
# id = '/lidar/velodyne/scan/1/lab/test1'
# r = LidarReader(id)
