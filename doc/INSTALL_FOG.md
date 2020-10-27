## Step 1: Install the OS

https://yanwei-liu.medium.com/nvidia-jetson-tx2%E5%AD%B8%E7%BF%92%E7%AD%86%E8%A8%98-%E4%B8%80-3dab5640968e

## Step 2: Install the Docker

https://docs.docker.com/engine/install/

## Step 3: Install Cmake with version >= 3.18.3

https://cmake.org/download/

## Step 4: Installing Environment

1. Mount the data disk at `/opt/aiot`
2. Install all dependency

```
sudo apt update && sudo apt install -y libmosquitto-dev \
    libmosquittopp-dev libssl-dev liblz4-dev \
    build-essential pkg-config \
    cmake autoconf automake libtool curl make g++ unzip \
    python3.8-dev libboost-all-dev libyaml-cpp-dev libpcap-dev

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

## Step 5: Compiling this library

```
mkdir build
cmake -DIN_FOG=ON ..
make -j4
```

## Step 6: Install the python library

```
cd python
python3 setup.py build
pip3 install --user .
```

## Step 7: Setup the environment variable

**SPECIAL** This step is different in different fog node

Edit the `fog_side/env/fog_env.sh` file and change the value of it according to the expected setting

## Step 8: Setup the sensor service parameter

Currently, we assume each fog node only have 1 sensor for each sensor type

In each sensor directory, edit the `xxx_deployer.pb.txt` according to the expected setting. May alos edit the `systemd/xxx.service` if necessary

## Step 9: Install this library

```
cd build
sudo make install
```

## Step 10: Enable the environemt variable

In `~/.bashrc` add those line
```
source /etc/profile.d/fog_env.sh
```

## Step 11: Start the service for the sensor

Need to install the newest openconnect
The apt provided version is too old. Need compile from source
```
## First check whether the vpn work regularly
sudo service vpn_tunnel start
sudo service vpn_tunnel status
## If it work ok, register it
sudo systemctl enable vpn_tunnel

## Repeat the above step for each sensor. The service name of
## those sensor is : camera, radar, lidar
```

## Step 12: Setup the data backup cron job

```
cd fog_side/data_sync
bash install_cron.sh
```
