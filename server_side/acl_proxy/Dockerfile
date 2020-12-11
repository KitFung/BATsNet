FROM --platform=arm64 envoyproxy/envoy:v1.16.1

WORKDIR /opt/acl_proxy
COPY envoy.yaml .
COPY entry.sh .

ENTRYPOINT ["/opt/acl_proxy/entry.sh"]
