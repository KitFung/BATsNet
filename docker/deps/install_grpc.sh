# protobuf
curl -LO https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protobuf-all-3.12.3.tar.gz
tar zxvf protobuf-all-3.12.3.tar.gz
pushd protobuf-3.12.3/
./autogen.sh
./configure
make -j4
make install
ldconfig
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
make install
popd
popd
