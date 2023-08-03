ARG DLLWorkerImage
FROM $DLLWorkerImage as source

#FROM ubuntu:23.04 AS builder
FROM alpine AS builder
# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
RUN apk update && apk add --no-cache \
    g++ \
    make\
    cmake \
    linux-headers

#RUN apt-get update && DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install -y \
#    ssh \
#    gcc \
#    g++ \
#    gdb \
#    clang \
#    make \
#    ninja-build \
#    cmake \
#    autoconf \
#    automake \
#    locales-all \
#    build-essential \
#    libc-ares-dev \
#    protobuf-compiler-grpc \
#    grpc-proto \
#    libgrpc-dev \
#    libgrpc++-dev \
#    libprotobuf-dev \
#    git \
#	&& apt-get clean

RUN mkdir -p /app/build
COPY --from=source /app/install/lib/ /app/install/lib/
COPY --from=source /app/install/include/armonik/sdk/worker/ /app/install/include/armonik/sdk/worker/
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DLLWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install
