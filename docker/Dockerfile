FROM nvcr.io/nvidia/l4t-ml:r32.4.3-py3

LABEL maintainer="KitFung"

WORKDIR /opt

COPY docker/deps/install_predeps.sh /opt
RUN bash /opt/install_predeps.sh

COPY docker/deps/install_pybind.sh /opt
RUN bash /opt/install_pybind.sh

COPY docker/deps/install_cmake.sh /opt
RUN bash /opt/install_cmake.sh

COPY docker/deps/install_grpc.sh /opt
RUN bash /opt/install_grpc.sh

COPY docker/deps/install_cpprest.sh /opt
RUN bash /opt/install_cpprest.sh

COPY . /opt/BATsNET/
RUN bash /opt/BATsNET/docker/compile_project.sh
