import lamppost.transport_py as transport_py
import lamppost.service_discovery_py as service_discovery_py

class DataReader(object):

    def __init__(self, DataType, data_identifier):
        self.DataType = DataType
        self.channel = \
            transport_py.BlockChannel(data_identifier)

    def read(self):
        data = self.channel.recv()
        parsed_data = self.DataType()
        parsed_data.ParseFromString(data)
        return parsed_data


class VideoStream(object):
    def __init__(self, data_identifier):
        self.data_identifier = data_identifier
    
    def get_stream_path(self):
        return service_discovery_py.get_service_path(
            self.data_identifier)

    def read_frame(self):
        # @TODO Not sure which reader is the best, so not impl yet
        # let the user to select his own reader
        pass
