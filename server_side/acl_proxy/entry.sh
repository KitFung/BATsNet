#!/bin/bash

# Start Proxy
cat /opt/acl_proxy/envoy.yaml | envsubst \$ACL_ADDRESS,\$ACL_PORT > /opt/acl_proxy/envoy.yaml
/usr/local/bin/envoy -c /opt/acl_proxy/envoy.yaml
