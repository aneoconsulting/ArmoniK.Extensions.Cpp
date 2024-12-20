# Start with the latest Alpine base image for the build stage
FROM dockerhubaneo/armonikworker_base:ubi7.9-0.0.1 AS builder

USER root
# Update the PATH environment variable to include the gRPC libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo && \
    sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo && \
    sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo

RUN yum --disableplugin=subscription-manager check-update \
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
ARG API_VERSION
RUN git clone https://github.com/aneoconsulting/ArmoniK.Api.git -b "${API_VERSION}" && \
    cd ArmoniK.Api/packages/cpp && \
    mkdir -p /app/proto && \
    mkdir -p /armonik/api && \
    cp -r ../../Protos/V1/* /app/proto && \
    mkdir -p build/ && \
    cd build/ && \
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" \
        "-DCMAKE_PREFIX_PATH=/usr/local/grpc" \
        "-DBUILD_TEST=OFF" \
        "-DBUILD_CLIENT=OFF" \
        "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install && \
    make clean


# Copy the application source files into the image
WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./ArmoniK.SDK.DynamicWorker ./ArmoniK.SDK.DynamicWorker
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

WORKDIR /app/builder/worker
ARG WORKER_VERSION
RUN cmake -DCMAKE_INSTALL_PREFIX=/app/install \
    -DCMAKE_PREFIX_PATH=/usr/local/grpc \
    -DARMONIK_API_DIR=/armonik/api \
    -DBUILD_CLIENT=OFF \
    -DBUILD_DYNAMICWORKER=ON \
    -DBUILD_END2END=OFF \
    -DVERSION="${WORKER_VERSION}" \
    /app/source/ && make -j $(nproc) install && make clean

# Start with the latest Alpine base image for the final stage
FROM dockerhubaneo/armonikworker_base:ubi7.9-0.0.1 AS runner

USER armonikuser

# Copy the application files, libraries, and binaries from the builder image to the final image
COPY --from=builder /app/install/. /app/install/.

# Update the PATH environment variable to include the application libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

# Set the entrypoint for the application's test executable
# This is the command that will be executed when the container is run
WORKDIR /app/install/bin
ENTRYPOINT ["./ArmoniK.SDK.DynamicWorker"]
