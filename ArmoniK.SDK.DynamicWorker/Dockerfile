# Start with the latest Alpine base image for the build stage
FROM alpine:3.18.6 AS builder

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
ARG API_VERSION
RUN git clone https://github.com/aneoconsulting/ArmoniK.Api.git -b "${API_VERSION}" && \
    cd ArmoniK.Api/packages/cpp && \
    mkdir -p /app/proto && \
    mkdir -p /armonik/api && \
    cp -r ../../Protos/V1/* /app/proto && \
    mkdir -p build/ && \
    cd build/ && \
    cmake "-DCMAKE_INSTALL_PREFIX=/armonik/api" "-DBUILD_TEST=OFF" "-DBUILD_CLIENT=OFF" "-DBUILD_WORKER=ON" .. && \
    make -j $(nproc) install && \
    make clean


# Copy the application source files into the image
WORKDIR /app/source
COPY tools/packaging/common/. ./tools/packaging/common/
COPY ./ArmoniK.SDK.Common ./ArmoniK.SDK.Common
COPY ./ArmoniK.SDK.Worker ./ArmoniK.SDK.Worker
COPY ./ArmoniK.SDK.DynamicWorker ./ArmoniK.SDK.DynamicWorker
COPY ./CMakeLists.txt ./
COPY ./Utils.cmake ./
COPY ./Packaging.cmake ./

# Build the application using the copied source files and protobuf definitions
WORKDIR /app/builder/worker
ARG WORKER_VERSION
RUN cmake -DCMAKE_INSTALL_PREFIX="/app/install" -DBUILD_CLIENT=OFF -DBUILD_DYNAMICWORKER=ON -DBUILD_END2END=OFF -DVERSION="${WORKER_VERSION}" /app/source/ && make -j $(nproc) install && make clean

# Start with the latest Alpine base image for the final stage
FROM alpine:3.18.6 AS runner
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
COPY --from=builder /app/install/include /app/install/include

# Update the PATH environment variable to include the application libraries and binaries
ENV LD_LIBRARY_PATH="/app/install/lib:$LD_LIBRARY_PATH"
ENV PATH="/app/install/bin:$PATH"

# Set the entrypoint for the application's test executable
# This is the command that will be executed when the container is run
WORKDIR /app/install/bin
ENTRYPOINT ["./ArmoniK.SDK.DynamicWorker"]
