cd ..
rm -rf build
mkdir build
cd build
cmake -DIN_FOG=ON -DINSTALL_DRIVER=OFF -DIN_DOCKER=ON ..
make -j4
