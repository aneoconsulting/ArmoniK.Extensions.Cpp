FROM registry.access.redhat.com/ubi8 AS builder

ARG PARALLEL_JOBS=4

RUN rpm -ivh https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

RUN yum --disableplugin=subscription-manager update -y && \
    yum --disableplugin=subscription-manager install -y \
    make \
    cmake \
    git \
    gcc \
    gcc-c++ \
    wget \
    rpm-build \
    libcurl-devel \
    fmt-devel \
    simdjson-devel \
    re2-devel \
    zlib-devel \
    openssl-devel

RUN yum --disableplugin=subscription-manager clean all

WORKDIR /tmp

# Fetch aneo's grpc rpm packages
RUN wget https://github.com/aneoconsulting/grpc-rpm/releases/download/1.62.2.0/grpc-1.62.2-1.el8.x86_64.rpm && \
    rpm -ivh grpc-1.62.2-1.el8.x86_64.rpm

RUN wget https://github.com/aneoconsulting/grpc-rpm/releases/download/1.62.2.0/grpc-devel-1.62.2-1.el8.x86_64.rpm && \
    rpm -ivh grpc-devel-1.62.2-1.el8.x86_64.rpm

RUN rm -rf *.rpm

# Compile API
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
        "-DSTATIC_LINK=ON" \        
        "-DBUILD_TEST=ON" \
        "-DBUILD_CLIENT=ON" \
        "-DBUILD_WORKER=ON" .. && \
    make -j ${PARALLEL_JOBS} install && \
    ls -alR /armonik/api && \
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
    /app/source/ && make -j ${PARALLEL_JOBS} && make install && make clean

# Start with the latest Alpine base image for the final stage
FROM registry.access.redhat.com/ubi8 AS runner

RUN yum --disableplugin=subscription-manager update -y && \
    yum --disableplugin=subscription-manager install -y \
    wget \
    openssl

RUN yum --disableplugin=subscription-manager clean all

WORKDIR /tmp
RUN wget https://github.com/aneoconsulting/grpc-rpm/releases/download/1.62.2.0/grpc-1.62.2-1.el8.x86_64.rpm && \
    rpm -ivh grpc-1.62.2-1.el8.x86_64.rpm && rm grpc-1.62.2-1.el8.x86_64.rpm

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
