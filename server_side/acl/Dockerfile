FROM --platform=linux/amd64 ubuntu:16.04 as build

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ \
        ca-certificates \
        wget && \
    rm -rf /var/lib/apt/lists/*

ENV GOLANG_VERSION 1.15.3
RUN wget -nv -O - https://golang.org/dl/go${GOLANG_VERSION}.linux-amd64.tar.gz \
    | tar -C /usr/local -xz
ENV GOPATH /go
ENV PATH $GOPATH/bin:/usr/local/go/bin:$PATH

WORKDIR /go/src/acl
COPY . .

WORKDIR /go/src/acl/acl

RUN export CGO_LDFLAGS_ALLOW='-Wl,--unresolved-symbols=ignore-in-object-files' && \
    /usr/local/go/bin/go install -ldflags="-s -w" -v .


FROM --platform=linux/amd64 debian:stretch-slim

COPY --from=build /go/bin/acl /usr/bin/acl

RUN mkdir -p /opt/acl

ENTRYPOINT ["acl"]
