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
    python3.8-dev libboost-all-dev

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
make -j8
sudo make install
popd
popd


# casablanca
sudo apt-get install g++ git libboost-atomic-dev \
 libboost-thread-dev libboost-system-dev libboost-date-time-dev \
 libboost-regex-dev libboost-filesystem-dev libboost-random-dev \
 ibboost-chrono-dev libboost-serialization-dev \
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

- mosquitto (libmosquitto-dev, libmosquittopp-dev)
- Protocol Buffers v3.12.3
- openssl (libssl-dev)
- lz4 (liblz4-dev)
- pybind11 (install via conda or build from source)
- python3 (recommend 3.8)
- grpc https://grpc.io/docs/languages/cpp/quickstart/
- etcd-cpp https://github.com/nokia/etcd-cpp-apiv3