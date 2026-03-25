# Use the latest version of Ubuntu 20.04 as the base image
FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install -y \
    gcc \
    g++ \
    make \
    build-essential \
    cmake \
    libc-ares-dev \
    libgrpc-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for protobuf
ENV protobuf_BUILD_TESTS=OFF

# Print the PATH variable
RUN echo $PATH

# Set the working directory for building protobuf
WORKDIR /app/build

# Set the default command to build the client using CMake and make
CMD ["bash", "-c", "cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/app/install -DBUILD_END2END=OFF /app/source/ && make install -j $(nproc)"]
