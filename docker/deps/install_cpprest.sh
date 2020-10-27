# casablanca
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
pushd casablanca
mkdir build.release
pushd build.release
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
ninja
ninja install
popd
popd
rm -rf casablanca
