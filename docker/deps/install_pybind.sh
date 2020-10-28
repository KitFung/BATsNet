# pybind11
/usr/bin/python3.6 -m pip install pytest
git clone https://github.com/pybind/pybind11.git
pushd pybind11
mkdir build
pushd build
cmake -DPYBIND11_PYTHON_VERSION=3.6 .. && make -j
make install
popd
popd
rm -rf pybind11
