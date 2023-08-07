ARG DynamicWorkerImage
FROM $DynamicWorkerImage as source

FROM dockerhubaneo/armonikworker_base:ubi7.9-0.0.1 AS builder

RUN mkdir -p /app/build
COPY --from=source /app/install/lib/ /app/install/lib/
COPY --from=source /app/install/include/armonik/sdk/worker/ /app/install/include/armonik/sdk/worker/
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install
