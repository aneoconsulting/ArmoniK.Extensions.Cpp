# ArmoniK.Extensions.Cpp

This project is part of the [ArmoniK](https://github.com/aneoconsulting/ArmoniK) project.

In order to build the SDK C++, one may have the ArmoniK API C++ installed in the local environnement, or use the script available to build in a docker environnement.

## How to build for Linux

### Prerequisites Linux

1. Install Docker on your Linux system. Follow the instructions on the [official Docker documentation](https://docs.docker.com/engine/install/).
2. Clone the repository containing the source code and the necessary scripts.

### CLion setup

- In WSL:
  - run ```restart-build-env.sh```
  - Specify the ARMONIK_API_DIR as an environnement variable prior to running this script if the ArmoniK.Api cpp installation directory is not ```../ArmoniK.Api/packages/cpp/install```
- In your IDE:
  - Setup ``Remote Host`` as your toolchain and specify the user as ``user``, password as ``password`` and the adress and ports to match the REMOTE_BUILD_ADDRESS variable specified in the script

### Visual Studio setup

- In WSL, run ```setup-remote.sh```
  - You will be connected to the docker in interactive mode. Then, you can build and run your project in the linux enviroment.
  - If You want to perform remote developpement in your IDE, then run:
  - ```service ssh start```
- In your IDE:
  - Setup ``Remote Host`` by specifying the user as ``root``, password as ``password`` and the ports to 2223 as in the script.
  - As the project source are mounted in the docker, you may want to disable the files copy for ``rsync``.

## How to build for Windows (Client only)

### Prerequisites Windows

- CMake
- gRPC
- ArmoniK API installed

### Visual Studio

The Windows build is only for client side not the worker. When the prerequisites are satisfied, setup your IDE and configure with CMake.

## How to use the ArmoniK C++ SDK

### Build worker and dynamic worker (Linux only)

In order to build the dynamic worker image (```armonik-sdk-cpp-dynamicworker```) and the worker test (```libArmoniK.SDK.Worker.Test.so```), one should run the following script:
    - ```build-deploy-end2end.sh```

This script will build the dynamic worker image to be used by the ArmoniK deployment and the worker shared library that provides the mandatory symbols. The script will also copy this shared library into the right folder so it can be loaded by the ArmoniK worker.

### Build the client test and submit jobs

Once the worker is built and deployed with ArmoniK, one can submit jobs. In order to do so, one may build the client test by launching the cmake with the flags ```BUILD_CLIENT=ON``` and ```BUILD_END2END=ON```. After the test build, the environnement variable ```Grpc__EndPoint``` must be set in the test environnement and then run:
    - ```./ArmoniK.SDK.Client.Test```
