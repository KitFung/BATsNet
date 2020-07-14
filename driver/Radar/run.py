import ctypes
import os
import time

from utils.RadarReader import RadarReader

libc = ctypes.CDLL("libc.so.6")


def main():
    folderpath = time.strftime('./data')

    if not os.path.exists(folderpath):
        os.mkdir(folderpath)

    duration = 30
    radar_fps = 20

    while True:
        libc.usleep(1)
        radar = RadarReader('./cfg/a.cfg', folderpath, duration * radar_fps)
        radar.run()


if __name__ == '__main__':
    main()
