# pybind11
/usr/bin/python3.8 -m pip install pytest
git clone https://github.com/pybind/pybind11.git
pushd pybind11
mkdir build
pushd build
cmake .. && make -j
make install
popd
popd
rm -rf pybind11
