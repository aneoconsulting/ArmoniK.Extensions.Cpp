# Use the latest version of Ubuntu 20.04 as the base image
FROM ubuntu:23.04

# Install dependencies
RUN apt-get update && DEBIAN_FRONTEND="noninteractive" TZ="Europe/London" apt-get install -y \
    gcc \
    g++ \
    make \
    build-essential \
    libfmt-dev \
    cmake \
    ssh \
    gdb \
    ninja-build \
    rsync \
    zip \
    openssh-server \
    libc-ares-dev \
    protobuf-compiler-grpc \
    grpc-proto \
    libgrpc-dev \
    libgrpc++-dev

# Set environment variables for protobuf
ENV protobuf_BUILD_TESTS=OFF

# configure SSH for communication with Visual Studio 
RUN mkdir -p /var/run/sshd

RUN echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \ 
   ssh-keygen -A 

RUN echo 'PermitRootLogin yes #enabled' >> /etc/ssh/sshd_config

# expose port 22 
EXPOSE 22

# Set the working directory for building protobuf
WORKDIR /app/build

RUN yes password | passwd root

# Set the default command to build the client using CMake and make
# CMD ["bash", "-c", "cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/app/install -DBUILD_CLIENT=ON -DBUILD_WORKER=ON -DBUILD_TEST=ON /app/source/ && make -j $(nproc) install"]

# Uncomment the line below if you want the container to start in the bash shell by default
ENTRYPOINT ["bash"]
