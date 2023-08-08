ARG DynamicWorkerImage
FROM $DynamicWorkerImage as source

FROM ubuntu:23.04 AS builder
# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
RUN apt-get update && DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install -y\
    g++ \
    make\
    cmake \
    && apt-get clean

# Cross Images instruction
RUN mkdir -p /app/build
COPY --from=source /app/install /app/install
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install
