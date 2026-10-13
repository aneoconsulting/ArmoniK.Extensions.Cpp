# ArmoniK C++ SDK for Windows (MSVC, x64)

This archive holds the ArmoniK C++ SDK (`ArmoniK.SDK.Common`, `ArmoniK.SDK.Client`) and the
ArmoniK.Api C++ client it is built on: headers, static libraries and CMake package files.
gRPC, protobuf and their dependencies are not included, they come from conda-forge.

## Requirements

- Visual Studio 2022 or later with the C++ workload, CMake 3.22 or later.
- [micromamba](https://mamba.readthedocs.io/en/latest/installation/micromamba-installation.html)
  (or conda / mamba) to install the dependencies.

## Install the dependencies

The ArmoniK.Api headers are generated with a specific protobuf version, so use the pinned
environment shipped next to this file:

```powershell
micromamba create -f environment.yml
```

## Build an application

```powershell
$deps = "$env:MAMBA_ROOT_PREFIX\envs\armoniksdk\Library"
cmake -S . -B build "-DCMAKE_PREFIX_PATH=C:\path\to\this\folder;$deps"
cmake --build build --config Release
```

In `CMakeLists.txt`:

```cmake
find_package(ArmoniK.SDK.Client CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ArmoniK.SDK.Client)
```

Only `Release` and `RelWithDebInfo` builds are supported. The SDK and the conda-forge packages
use the release runtime (`/MD`), so linking them in a `Debug` build fails with LNK2038.

## Run an application

The gRPC, protobuf and abseil DLLs must be found at run time. Add `$deps\bin` to `PATH`, or copy
the DLLs next to the executable.
