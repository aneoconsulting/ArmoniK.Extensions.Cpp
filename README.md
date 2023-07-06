# ArmoniK.Extensions.Cpp

## How to build for Linux

- In WSL, run ```restart-build-env.sh```
  - Specify the ARMONIK_API_DIR as an environnement variable prior to running this script if the ArmoniK.Api cpp installation directory is not ```../ArmoniK.Api/packages/cpp/install```
- In your IDE:
  - For Clion, setup ``Remote Host`` as your toolchain and specify the user as ``user``, password as ``password`` and the adress and ports to match the REMOTE_BUILD_ADDRESS variable specified in the script
  - For Visual Studio ...

## How to build for Windows

Client only !

...