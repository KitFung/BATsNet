# protobuf
curl -LO https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protobuf-all-3.12.3.tar.gz
tar zxvf protobuf-all-3.12.3.tar.gz
rm -rf protobuf-all-3.12.3.tar.gz
pushd protobuf-3.12.3/
./autogen.sh
./configure
# ./configure --prefix=/usr/lib/aarch64-linux-gnu
make -j4
make install
ldconfig
popd
rm -rf protobuf-3.12.3/
