FROM debian:12

# Install debian package making tools
RUN apt-get update && DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install --no-install-recommends -y \
    devscripts \
    equivs \
    git \
    && apt-get clean

WORKDIR /app/source
COPY tools/packaging/debian/control ./tools/packaging/debian/control
# Install build dependencies
RUN yes | mk-build-deps -i -r -B ./tools/packaging/debian/control

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
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" "-DBUILD_TEST=OFF" "-DBUILD_CLIENT=ON" "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install && \
    make clean

WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Client ./ArmoniK.SDK.Client
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

WORKDIR /app/libarmonik/build
ARG WORKER_VERSION
RUN cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/app/install -DINSTALL_SDK_DIR=/app/install -DARMONIK_API_DIR=/armonik/api -DCPACK_GENERATOR=DEB -DBUILD_DYNAMICWORKER=OFF -DBUILD_END2END=OFF -DVERSION="${WORKER_VERSION}" /app/source/ && make -j $(nproc) install && make package -j
ENTRYPOINT ["bash"]
