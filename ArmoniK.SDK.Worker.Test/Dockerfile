ARG DLLWorkerImage
FROM $DLLWorkerImage as source

FROM alpine AS builder
# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
RUN apk update && apk add --no-cache \
    g++ \
    make\
    cmake \
    linux-headers

RUN mkdir -p /app/build
COPY --from=source /app/install/ArmoniKSDKInterface.h /app/install/
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DLLWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install