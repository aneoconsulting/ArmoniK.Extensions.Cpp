ARG DynamicWorkerImage
ARG BuildBaseImage
FROM $DynamicWorkerImage as source

ARG BuildBaseImage
FROM $BuildBaseImage AS builder
USER root
# Install all the necessary dependencies required for the build process
# These include tools and libraries for building and compiling the source code
COPY --chmod=755 ./ArmoniK.SDK.Worker.Test/install-dependencies.sh ./install-dependencies.sh
RUN ./install-dependencies.sh
# Force an interactive shell
SHELL ["/bin/bash", "--login", "-c"]

# Cross Images instruction
RUN mkdir -p /app/build
COPY --from=source /app/install /app/install
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install && make clean
