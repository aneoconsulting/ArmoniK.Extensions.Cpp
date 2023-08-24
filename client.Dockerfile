# Start with the latest Alpine base image for the build stage
FROM alpine AS builder

# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
RUN apk update && apk add --no-cache \
    git \
    gcc \
    g++ \
    build-base \
    libtool \
    curl \
    c-ares \
    c-ares-dev \
    make \
    cmake \
    linux-headers \
    grpc \
    grpc-dev \
    protobuf \
    protobuf-dev

# Update the PATH environment variable to include the gRPC libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

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
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" "-DBUILD_TEST=OFF" "-DBUILD_CLIENT=ON" "-DBUILD_WORKER=OFF" .. && \
    make -j $(nproc) install && \
    make clean


# Copy the application source files into the image
WORKDIR /app/source
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Client ./ArmoniK.SDK.Client
COPY ./ArmoniK.SDK.Client.Test ./ArmoniK.SDK.Client.Test
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./

# Build the application using the copied source files and protobuf definitions
WORKDIR /app/build
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/install" "-DINSTALL_SDK_DIR=/app/install" "-DBUILD_WORKER=OFF" "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install && make clean

# Set the entrypoint for the application's test executable
# This is the command that will be executed when the container is run
ENTRYPOINT ["/app/install/bin/ArmoniK.SDK.Client.Test"]
