FROM registry.access.redhat.com/ubi8

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

RUN git clone --depth 1 https://github.com/nlohmann/json.git -b v3.11.3 /tmp/nlohmann-json && \
    cmake -S /tmp/nlohmann-json -B /tmp/nlohmann-json/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DJSON_BuildTests=OFF && \
    cmake --build /tmp/nlohmann-json/build --target install && \
    rm -rf /tmp/nlohmann-json

WORKDIR /tmp

# Fetch aneo's grpc rpm packages, in our repo we append an extra .x to the grpc version to distinguish several builds of the
# same version.
ARG GRPC_VERSION=1.62.2
ARG GRPC_BUILD=${GRPC_VERSION}.0

RUN wget "https://github.com/aneoconsulting/grpc-rpm/releases/download/${GRPC_BUILD}/grpc-${GRPC_BUILD%.*}-1.el8.x86_64.rpm" && \
    rpm -ivh "grpc-${GRPC_BUILD%.*}-1.el8.x86_64.rpm"

RUN wget "https://github.com/aneoconsulting/grpc-rpm/releases/download/${GRPC_BUILD}/grpc-devel-${GRPC_BUILD%.*}-1.el8.x86_64.rpm" && \
    rpm -ivh "grpc-devel-${GRPC_BUILD%.*}-1.el8.x86_64.rpm"

# Default value read from tools/common.sh file
ARG API_VERSION
RUN wget "https://github.com/aneoconsulting/ArmoniK.Api/releases/download/${API_VERSION}/libarmonik-${API_VERSION}-Linux.rpm" && \
    rpm -ivh "libarmonik-${API_VERSION}-Linux.rpm"

RUN rm -rf *.rpm

# Copy the application source files into the image
WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Client ./ArmoniK.SDK.Client
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

WORKDIR /app/build
ARG WORKER_VERSION
ARG CPACK_GENERATOR
RUN cmake "-DBUILD_SHARED_LIBS=ON" \
    "-DCMAKE_BUILD_TYPE=Release" \
    "-DCMAKE_INSTALL_PREFIX=/app/install" \
    "-DINSTALL_SDK_DIR=/app/install" \
    "-DCMAKE_PREFIX_PATH=/usr/local/grpc" \
    "-DARMONIK_API_DIR=/armonik/api" \
    "-DBUILD_DYNAMICWORKER=OFF" \
    "-DBUILD_END2END=OFF" \
    "-DCPACK_GENERATOR=${CPACK_GENERATOR}" \
    "-DVERSION=${WORKER_VERSION}" \
    /app/source/ && \
    make -j ${PARALLEL_JOBS} install && \
    make package

# Set the default command to build the client using CMake and make
CMD ["bash"]
