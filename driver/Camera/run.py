import ctypes
import os

from utils.CameraReader import CameraReader

libc = ctypes.CDLL("libc.so.6")


def main():
    source = "rtsp://admin:75#=i72i@10.32.10.2:554/Streaming/Channels/1"
    FPS = 30
    while True:
        libc.usleep(1)
        camera_reader = CameraReader(source, FPS)
        camera_reader.record()


if __name__ == '__main__':
    main()
