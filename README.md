# BATsNet

The codebase of the smart lamppost system.

Used in the BAT side: `bat_side`

Used in the server side: `server_side`

Provide the underlying function and interface in fog side: `common`, `data_collector`, `device`, `python`, `scheduler`, `transport`, `service_discovery`

Related to the sensor: `camera`, `radar_iwr6843`, `lidar`

Used in fog side, but not used by the user task or sensor: `fog_size`

# Download this library
```
git clone --recurse-submodules -j8 git://github.com/KitFung/BATsNet
```

## Dependency
----

```
sudo apt update && sudo apt install -y libmosquitto-dev \
    libmosquittopp-dev libssl-dev liblz4-dev \
    build-essential pkg-config \
    cmake autoconf automake libtool curl make g++ unzip \
    python3.6-dev libboost-all-dev libyaml-cpp-dev libpcap-dev

# protobuf
curl -LO https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protobuf-all-3.12.3.tar.gz
tar zxvf protobuf-all-3.12.3.tar.gz
pushd protobuf-3.12.3/
./autogen.sh
./configure
make
sudo make install
sudo ldconfig
popd

# pybind11
git clone https://github.com/pybind/pybind11.git
pushd pybind11
mkdir build
pushd build
cmake .. && make -j
sudo make install
popd
popd

# grpc
git clone https://github.com/grpc/grpc.git
pushd grpc/
git submodule update --init
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON       -DgRPC_BUILD_TESTS=OFF  -DBUILD_SHARED_LIBS=ON     ../..
# cmake -DgRPC_INSTALL=ON -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_BUILD_TESTS=OFF  -DBUILD_SHARED_LIBS=ON     ../.. 
make -j8
sudo make install
popd
popd


# casablanca
sudo apt-get install g++ git libboost-atomic-dev \
 libboost-thread-dev libboost-system-dev libboost-date-time-dev \
 libboost-regex-dev libboost-filesystem-dev libboost-random-dev \
 libboost-chrono-dev libboost-serialization-dev libboost-locale-dev \
 libwebsocketpp-dev openssl libssl-dev ninja-build
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
pushd casablanca
mkdir build.release
pushd build.release
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
ninja
sudo ninja install
popd
popd


```

- libyaml-cpp-dev, libpcap-dev
- mosquitto (libmosquitto-dev, libmosquittopp-dev)
- Protocol Buffers v3.12.3
- openssl (libssl-dev)
- lz4 (liblz4-dev)
- pybind11 (install via conda or build from source)
- python3.6
- grpc https://grpc.io/docs/languages/cpp/quickstart/
- etcd-cpp https://github.com/nokia/etcd-cpp-apiv3
