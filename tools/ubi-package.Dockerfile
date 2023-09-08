# Start with the latest Alpine base image for the build stage
FROM dockerhubaneo/armonikworker_base:ubi7.9-0.0.1 AS builder

USER root
# Update the PATH environment variable to include the gRPC libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

RUN yum check-update \
    ; yum --disableplugin=subscription-manager \
        install -y git make \
        rh-python38-python-devel \
        centos-release-scl \
        devtoolset-10 \
        && yum --disableplugin=subscription-manager clean all

RUN  ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin

RUN echo "source /opt/rh/devtoolset-10/enable" >> /etc/bashrc
SHELL ["/bin/bash", "--login", "-c"]

ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH"
# Generate unique machne-id file required by dbus-11
RUN dbus-uuidgen > /var/lib/dbus/machine-id

# Display the updated PATH environment variable
RUN echo $PATH

# Get and install ArmoniK api into the image
WORKDIR /tmp
ARG API_VERSION=3.11.0
RUN git clone https://github.com/aneoconsulting/ArmoniK.Api.git && \
    cd ArmoniK.Api/packages/cpp && \
    git checkout "${API_VERSION}" && \
    mkdir -p /app/proto && \
    mkdir -p /armonik/api && \
    cp -r ../../Protos/V1/* /app/proto && \
    mkdir -p build/ && \
    cd build/ && \
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" \
        "-DCMAKE_PREFIX_PATH=/usr/local/grpc" \
        "-DBUILD_TEST=OFF" \
        "-DBUILD_CLIENT=ON" \
        "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install && \
    ls -alR /armonik/api && \
    make clean

WORKDIR /app/build
ENV CXX = "/opt/rh/devtoolset-10/root/usr/bin/c++"
ARG WORKER_VERSION=0.1.0

# Set the default command to build the client using CMake and make
CMD ["bash", "-c", "cmake -DCMAKE_INSTALL_PREFIX=/app/install -DINSTALL_SDK_DIR=/app/install -DCMAKE_PREFIX_PATH=/usr/local/grpc -DARMONIK_API_DIR=/armonik/api -DBUILD_DYNAMICWORKER=OFF -DBUILD_END2END=OFF /app/source/ && make -j $(nproc) install && make package -j"]