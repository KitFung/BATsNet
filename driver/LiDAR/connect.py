import socket


class UdpClient(object):
    def __init__(self, ip, port):
        self._ip = ip
        self._port = port
        self._sock = None

    def _connect(self):
        pair = (self._ip, self._port)
        self._sock = socket.socket(
            socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind(pair)

    def run(self):
        self._connect()
        while True:
            data, addr = self._sock.recvfrom(2408)
            if not data:
                print("client has exist")
                break
            data = data.hex()
            print("received:", data, "with length", len(data), "from", addr)

    def terminate(self):
        self._sock.close()


def main():
    udpclient = UdpClient("192.168.1.246", 2368)
    udpclient.run()
    udpclient.terminate()


if __name__ == '__main__':
    main()
