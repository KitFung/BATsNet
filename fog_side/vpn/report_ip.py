import os
import sys
import netifaces
import etcd3
import time
from datetime import datetime

SERVER_IP = '137.189.97.26'
# SERVER_IP = '137.189.97.26'
# THIS_NODE_NAME = 'node1'
REGISTER_SECOND = int(sys.argv[1]) * 2
THIS_NODE_NAME = os.environ['NODE_NAME']
key = '/node/' + THIS_NODE_NAME


time_obj = datetime.now()
time_str = time_obj.strftime("%d-%b-%Y (%H:%M:%S.%f)")
if 'tun0' not in netifaces.interfaces():
    print('%s: VPN is disconnected, waiting it to restart' % time_str)
else:
    THIS_IP = netifaces.ifaddresses('tun0')[netifaces.AF_INET][0]['addr']
    try:
        etcd = etcd3.client(host=SERVER_IP, port=12379)
        etcd.put(key, THIS_IP, etcd.lease(REGISTER_SECOND))
        print('%s: Run OK' % time_str)
    except etcd3.exceptions.ConnectionFailedError as e:
        print('%s: Cannot Connect to Server' % time_str)

# import etcd3
# SERVER_IP = '137.189.97.26'
# etcd = etcd3.client(host=SERVER_IP, port=12379)
# res = etcd.get_prefix('/node/')
# for r in res:
#     print(r[0])
#     print(r[1].key)
