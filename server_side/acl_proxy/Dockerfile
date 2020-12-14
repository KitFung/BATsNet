FROM --platform=arm64 envoyproxy/envoy:v1.16.1

WORKDIR /opt/acl_proxy
COPY envoy.yaml .
COPY entry.sh .

RUN \
  apt-get update \
  && apt-get -y install gettext \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/opt/acl_proxy/entry.sh"]
