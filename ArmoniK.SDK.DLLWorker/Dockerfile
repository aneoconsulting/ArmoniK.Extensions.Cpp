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
RUN git clone https://github.com/aneoconsulting/ArmoniK.Api.git && \
    cd ArmoniK.Api/packages/cpp && \
    git checkout c64a5b8465cc2a4a0590b22cec215d8032dd2969 && \
    mkdir -p /app/proto && \
    mkdir -p /armonik/api && \
    cp -r ../../Protos/V1/* /app/proto && \
    mkdir -p build/ && \
    cd build/ && \
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" "-DBUILD_TEST=OFF" "-DBUILD_CLIENT=OFF" "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install 
    


# Copy the application source files into the image
WORKDIR /app/source
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./ArmoniK.SDK.DLLWorker ./ArmoniK.SDK.DLLWorker
COPY ./CMakeLists.txt ./

# Build the application using the copied source files and protobuf definitions
WORKDIR /app/builder/sdk
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/install" "-DBUILD_CLIENT=OFF" "-DBUILD_DLLWORKER=OFF" "-DBUILD_END2END=OFF" /app/source/ && make -j $(nproc) install

WORKDIR /app/builder/worker
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/install" "-DBUILD_CLIENT=OFF" "-DBUILD_DLLWORKER=ON" "-DBUILD_END2END=OFF" /app/source/ && make -j $(nproc) install

# Start with the latest Alpine base image for the final stage
FROM alpine AS runner
# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
RUN apk update && apk add --no-cache \
    c-ares \
    libprotobuf \
    grpc \
    grpc-cpp
	
# Create a non-root user and group for running the application
# This is a security best practice to avoid running applications as the root user
RUN addgroup -g 5000 -S armonikuser && adduser -D -h /home/armonikuser  -u 5000 -G armonikuser --shell /bin/sh armonikuser && mkdir /cache && chown armonikuser: /cache
USER armonikuser

# Copy the application files, libraries, and binaries from the builder image to the final image
COPY --from=builder /app/install/bin /app/install/bin
COPY --from=builder /app/install/lib /app/install/lib

# Update the PATH environment variable to include the application libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

# Set the entrypoint for the application's test executable
# This is the command that will be executed when the container is run
ENTRYPOINT ["/app/install/bin/ArmoniK.SDK.DLLWorker"]
