ARG DynamicWorkerImage
FROM $DynamicWorkerImage AS source

FROM alpine:3.18.12 AS builder

RUN apk update && apk add --no-cache \
    g++ \
    make\
    cmake \
    linux-headers \
    bash

# Force an interactive shell
SHELL ["/bin/bash", "--login", "-c"]

# Cross Images instruction
RUN mkdir -p /app/build
COPY --from=source /app/install /app/install

WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Examples/hello/worker ./ArmoniK.SDK.Examples/hello/worker
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

ARG WorkerLibVersion
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/install" \
    "-DBUILD_SDK=OFF" "-DBUILD_CLIENT=OFF" "-DBUILD_WORKER=ON" \
    "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=OFF" \
    "-DBUILD_WORKERTEST=OFF" "-DBUILD_EXAMPLES=ON" "-DVERSION=${WorkerLibVersion}" \ 
    /app/source/ && make -j $(nproc) install && make clean