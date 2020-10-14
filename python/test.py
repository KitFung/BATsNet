import time
from lamppost import reader, velodyne_pb2

# # Test Video Stream
# stream = reader.VideoStream('/thermal/trafi_one_195/stream/1/lab/test1')
# print(stream.get_stream_path())

# Test Liar Read
reader = reader.DataReader(velodyne_pb2.VelodyneScan, '/lidar/velodyne/scan/1/lab/test1')
time.sleep(1)
for i in range(10):
    scan = reader.read()
    print("{:.16f}\n".format(scan.stamp))
