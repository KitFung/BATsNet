import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

IP="192.168.100.3"

# Bind the socket to the port
server_address = ('0.0.0.0', 3314)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)
while True:
    data, address = sock.recvfrom(4096)
    if data:
        sent = sock.sendto(IP, address)
