ARG DynamicWorkerImage
FROM $DynamicWorkerImage as source

FROM dockerhubaneo/armonikworker_base:ubi7.9-0.0.1 AS builder
USER root
RUN  ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin
RUN yum --disableplugin=subscription-manager check-update \
    ; yum --disableplugin=subscription-manager \
        install -y \
        centos-release-scl \
        devtoolset-10 \
        && yum --disableplugin=subscription-manager clean all
RUN echo "source /opt/rh/devtoolset-10/enable" >> /etc/bashrc
SHELL ["/bin/bash", "--login", "-c"]


RUN mkdir -p /app/build
COPY --from=source /app/install /app/install
WORKDIR /app/source
COPY ./ArmoniK.SDK.Worker.Test ./ArmoniK.SDK.Worker.Test
COPY ./CMakeLists.txt ./CMakeLists.txt
RUN cmake "-DCMAKE_INSTALL_PREFIX=/app/build" "-DBUILD_CLIENT=OFF" "-DBUILD_DYNAMICWORKER=OFF" "-DBUILD_END2END=ON" /app/source/ && make -j $(nproc) install
