import sys
import datetime
import time

import etcd3

# This simple service basically never crash, so allow longer time
REGISTER_SECOND = 30

fog_service_id = sys.argv[1]
envoy_address_port = sys.argv[2]
bats_ip=sys.argv[3]

# etcd = etcd3.client(host='0.0.0.0', port=2379)
etcd = etcd3.client(host=bats_ip, port=2379)
start_time = datetime.datetime.now()

while True:
    now = datetime.datetime.now()
    passed_second = (now - start_time).total_seconds()

    # Register this proxy
    etcd.put(envoy_address_port, fog_service_id, etcd.lease(REGISTER_SECOND))

    if passed_second > 30:
        # Check the fog alive, if fog down proxy also go down
        v, _ = etcd.get(fog_service_id)
        if v is None or v.decode() != envoy_address_port:
            print("Cannot Match the fog service. Shut down")
            print(v.decode())
            print(envoy_address_port)
            exit(1)
    time.sleep(REGISTER_SECOND / 2.0)
