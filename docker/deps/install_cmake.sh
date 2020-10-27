curl -LO https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4.tar.gz
tar zxf cmake-3.18.4.tar.gz
rm -rf cmake-3.18.4.tar.gz
pushd cmake-3.18.4
mkdir build
pushd build
cmake ..
make -j8
make install
update-alternatives \
    --install /usr/bin/cmake cmake \
    /usr/local/bin/cmake 1 --force
popd
popd
rm -rf cmake-3.18.4
