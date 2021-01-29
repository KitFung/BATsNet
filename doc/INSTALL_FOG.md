## Step 1: Install the OS

https://yanwei-liu.medium.com/nvidia-jetson-tx2%E5%AD%B8%E7%BF%92%E7%AD%86%E8%A8%98-%E4%B8%80-3dab5640968e

### Set disk
The disk mount: need add `nofail` to it option

Assume already setup the disk, then create file `/etc/init.d/mounta` with content
```
#! /bin/sh
### BEGIN INIT INFO
# Required-Start:    $all
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: Setup the device
### END INIT INFO
/bin/sleep 20
/sbin/swapoff -a
/sbin/route add -net 192.168.100.0/24 gw 10.0.0.100 dev eth0
/bin/mount -a
```
then chmod +x to that file

then 
```
sudo systemctl enable mounta
```

### Set network
then vim `/etc/network/interfaces`
add below

```
auto lo
iface lo inet loopback

auto eth0
allow-hotplug eth0
iface eth0 inet static
address 10.0.0.2
netmask 255.255.255.0
network 10.0.0.0
broadcast 10.0.0.255
```
then reboot

## Step 2: Install the Docker

https://docs.docker.com/engine/install/

Add this args to `/etc/docker/daemon.json`
```
{
    "default-runtime": "nvidia",
    "insecure-registries": [
        "137.189.97.26:5000"
    ]
}
```

## Step 3: Install Cmake with version >= 3.18.3

https://cmake.org/download/

```
sudo apt install -y libssl-dev

curl -LO https://github.com/Kitware/CMake/releases/download/v3.19.0-rc3/cmake-3.19.0-rc3.tar.gz
tar zxvf cmake-3.19.0-rc3.tar.gz
rm -rf cmake-3.19.0-rc3.tar.gz
pushd cmake-3.19.0-rc3
mkdir build
pushd build
cmake ..
make -j4
sudo make install
popd
popd
```

## Step 4: Installing Environment

1. Mount the data disk at `/opt/aiot`
2. Install all dependency

```
sudo apt update && sudo apt install -y libmosquitto-dev \
    libmosquittopp-dev libssl-dev liblz4-dev \
    build-essential pkg-config \
    cmake autoconf automake libtool curl make g++ unzip \
    python3.6-dev libboost-all-dev libyaml-cpp-dev libpcap-dev \
    openssh-server libc-ares2 libc-ares-dev

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
cmake .. && make -j4
sudo make install
popd
popd

# grpc
git clone https://github.com/grpc/grpc.git
pushd grpc/
git submodule update --init
mkdir -p cmake/build
pushd cmake/build
# cmake -DgRPC_INSTALL=ON       -DgRPC_BUILD_TESTS=OFF  -DBUILD_SHARED_LIBS=ON     ../..
cmake -DgRPC_INSTALL=ON -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_BUILD_TESTS=OFF  -DBUILD_SHARED_LIBS=ON     ../.. 
make -j8
sudo make install
popd
popd


# casablanca
sudo apt-get -y install g++ git libboost-atomic-dev \
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

## Step 4.5 Install etcd

```
sudo apt install etcd

sudo cat > /lib/systemd/system/etcd.service << EOF
[Unit]
Description=etcd - highly-available key value store
Documentation=https://github.com/coreos/etcd
Documentation=man:etcd
After=network.target
Wants=network-online.target

[Service]
Environment=DAEMON_ARGS=
Environment=ETCD_NAME=%H
Environment=ETCD_DATA_DIR=/var/lib/etcd/default
Environment=ETCD_UNSUPPORTED_ARCH=arm64
EnvironmentFile=-/etc/default/%p
Type=notify
User=root
PermissionsStartOnly=true
#ExecStart=/bin/sh -c "GOMAXPROCS=$(nproc) /usr/bin/etcd $DAEMON_ARGS"
ExecStart=/usr/bin/etcd $DAEMON_ARGS
Restart=on-abnormal
#RestartSec=10s
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
Alias=etcd2.service
EOF

sudo systemctl daemon-reload
sudo service etcd restart
sudo systemctl enable etcd
```

## Step 5: Compiling this library

```
# If haven't clone the submodule
# git submodule update --init
mkdir build
pushd build
cmake -DIN_FOG=ON ..
make -j4
popd

sudo usermod -a -G dialout nvidia
```

## Step 6: Install the python library

```
cd python
sudo apt install -y python3-pip
sudo pip3 install setuptools
bash prepare.sh
python3 setup.py build
sudo pip3 install --upgrade cython

pip3 install --user .

sudo -i
pip3 install .
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
# to avoid permission issue
sudo chmod -R 777 /opt/aiot
```

## Step 10: Enable the environemt variable

In `~/.bashrc` add those line
```
source /etc/profile.d/fog_env.sh
```

## Step 11: Start the service for the sensor

First setup the nameserver
```
sudo vim /etc/resolvconf/resolv.conf.d/head
```

Then add following line
```
nameserver 8.8.8.8 
nameserver 8.8.4.4
```

Need to install the newest openconnect
The apt provided version is too old. Need compile from source
```
sudo apt install vpnc-scripts
curl -LO https://github.com/openconnect/openconnect/archive/v8.10.tar.gz
tar zxvf v8.10.tar.gz
rm -rf v8.10.tar.gz
pushd openconnect-8.10
./autogen.sh
./configure
make -j8
sudo make install
sudo pip3 install netifaces etcd3
sudo ldconfig

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
sudo bash install_cron.sh

# Setup scp without password
ssh-keygen
ssh-copy-id -p 2202 testbed@137.189.97.26

sudo -i
ssh-keygen
ssh-copy-id -p 2202 testbed@137.189.97.26
```

## Step 13: Join the k8s network

edit `/etc/hosts`, add below line. The value need to be change according to the actual ip and node name of that node
```
10.10.54.212 node-0
```

```
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
EOF
sudo sysctl --system

sudo apt-get update && sudo apt-get install -y apt-transport-https curl
curl -s https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
cat <<EOF | sudo tee /etc/apt/sources.list.d/kubernetes.list
deb https://apt.kubernetes.io/ kubernetes-xenial main
EOF
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl
sudo apt-mark hold kubelet kubeadm kubectl
```
