import socket
import sys
import subprocess
import datetime
import os
# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

IP = "192.168.100.3"

ALLOC_PORT_START = 20000
ALLOC_PORT_END = 30000
PROXY_DIR = os.path.join(os.path.dirname(
    os.path.realpath(__file__)), 'grpc_proxy')


def get_ports():
    unusable_ports = set()
    out_bytes = subprocess.check_output(
        "netstat -tunlep | grep LISTEN | awk '{print $4}'", shell=True)
    for line in out_bytes.split(b'\n'):
        if len(line) < 3:
            continue
        _, port = line.rsplit(b':', 1)
        port = int(port)
        unusable_ports.add(port)

    for i in range(ALLOC_PORT_START, ALLOC_PORT_END):
        if i not in unusable_ports:
            return i


def alloc_port(id, fog_ip, fog_port):
    # Get Port
    port = get_ports()
    # Start the proxy
    cmd = [
        "docker",
        "run",
        "--rm",
        "-d",
        "--net=host",
        "--env", "ASSIGNED_PORT=%d" % port,
        "--env", "FOG_ADDRESS=%s" % fog_ip,
        "--env", "FOG_PORT=%s" % fog_port,
        "--env", "FOD_SERVICE_ID=%s" % id,
        "--env", "EXPECTED_KEY=%s:%s" % (IP, port),
        "-v", "%s:/grpc_proxy" % PROXY_DIR,
        "--name", "envoy_proxy_%d" % port,
        "kitfung/envoy_plus:v1.15-latest",
        "/grpc_proxy/start.sh"
    ]
    subprocess.Popen(cmd)
    return port


# Bind the socket to the port
server_address = ('0.0.0.0', 3314)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)
while True:
    data, address = sock.recvfrom(4096)
    if data:
        if data.strip() == 'GET_IP':
            sent = sock.sendto(IP, address)
        elif data.startswith('GET_PROXY_PORT|'):
            id = data.split('|')[1]
            address_ip = address[0]
            address_port = data.split('|')[2]
            port = alloc_port(id, address_ip, address_port)
            sent = sock.sendto(str(port), address)
