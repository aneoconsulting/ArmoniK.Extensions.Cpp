# Builds and runs the unit tests of the worker side, which need no cluster. The build fails if a test fails.
FROM alpine:3.18.12

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
    protobuf-dev \
    nlohmann-json

# ArmoniK.Api with the worker library, which ConventionExecutor and the DynamicWorker use
WORKDIR /tmp
ARG API_VERSION
RUN git clone --depth 1 https://github.com/aneoconsulting/ArmoniK.Api.git -b "${API_VERSION}" && \
    cd ArmoniK.Api/packages/cpp && \
    mkdir -p build/ && \
    cd build/ && \
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" "-DBUILD_TEST=OFF" "-DBUILD_CLIENT=OFF" "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install && \
    make clean

WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./ArmoniK.SDK.DynamicWorker ./ArmoniK.SDK.DynamicWorker
COPY ./ArmoniK.SDK.Worker.UnitTest ./ArmoniK.SDK.Worker.UnitTest
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

WORKDIR /app/build
RUN cmake -DBUILD_CLIENT=OFF -DBUILD_WORKER=ON -DBUILD_DYNAMICWORKER=ON \
    -DBUILD_UNITTEST=ON -DBUILD_END2END=OFF /app/source/ && \
    make -j $(nproc) ArmoniK.SDK.Worker.UnitTest

# The DynamicWorker test needs ArmoniK.Api.Worker at run time
ENV LD_LIBRARY_PATH="/armonik/api/lib"
RUN ./ArmoniK.SDK.Worker.UnitTest/ArmoniK.SDK.Worker.UnitTest
