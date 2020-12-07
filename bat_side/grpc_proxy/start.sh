#!/bin/bash

cd /grpc_proxy
cat envoy-proxy.yaml | envsubst \$ASSIGNED_PORT,\$FOG_ADDRESS,\$FOG_PORT > /home/envoy/proxy.yaml

# Start Proxy
/usr/local/bin/envoy -c /home/envoy/proxy.yaml &

# Start the Checking Proxy
python3 checkalive.py $FOG_SERVICE_ID $EXPECTED_KEY $BATS_FIP
