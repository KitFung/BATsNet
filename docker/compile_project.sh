# Make sure add the dependency is setup
rm /usr/lib/aarch64-linux-gnu/libprotobuf*
ldconfig

cd /opt/BATsNET

rm -rf build
mkdir build
cd build
cmake -DIN_FOG=ON -DINSTALL_DRIVER=OFF -DIN_DOCKER=ON ..
make -j
make install

cd ../python
bash prepare.sh
pip3 install .
