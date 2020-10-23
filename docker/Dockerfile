from nvcr.io/nvidia/l4t-ml:r32.4.3-py3

MAINTAINER KitFung

WORKDIR /opt

COPY docker/deps/ /opt
RUN bash /opt/install_predeps.sh
RUN bash /opt/install_pybind.sh
RUN bash /opt/install_grpc.sh
RUN bash /opt/install_cpprest.sh

WORKDIR /opt/BATsNET
COPY . /opt/BATsNET/
RUN bash /opt/BATsNET/docker/compile_project.sh
