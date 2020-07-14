import ctypes
import datetime
import os
import subprocess

libc = ctypes.CDLL("libc.so.6")


class CameraReader():
    def __init__(self, source, fps, duration=60, path="./data/", format=".mp4"):
        self._source = source
        self._fps = fps
        self._duration = duration
        self._path = path
        self._format = format

    def record(self, prefix="Camera"):
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
        command_string = 'openRTSP -D 1 -B 1000000 -b 1000000 -4 -Q -f {} -F {} -d 60 -t {} > "{}"'.format(
            str(self._fps), prefix, self._source, self._path + timestamp + self._format)
        command = subprocess.call([command_string], shell=True)

    def record_slice(self, prefix="./data/Slice-"):
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
        command_string = 'openRTSP -D 1 -B 1000000 -b 1000000 -m -Q -F {} -d 10 -t {}'.format(
            prefix, self._source)
        command = subprocess.call([command_string], shell=True)
