#!/bin/bash

# Start Proxy
cat /opt/acl_proxy/envoy.yaml | envsubst \$ACL_ADDRESS,\$ACL_PORT > /opt/acl_proxy/envoy_var.yaml

cat /opt/acl_proxy/envoy_var.yaml

/usr/local/bin/envoy -c /opt/acl_proxy/envoy_var.yaml
