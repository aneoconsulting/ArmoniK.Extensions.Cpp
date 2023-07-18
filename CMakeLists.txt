cmake_minimum_required(VERSION 3.22)

set(VERSION 0.1.0)

set(SOLUTION_NAME ArmoniK.SDK)
project(${SOLUTION_NAME} C CXX)

if (UNIX)
    set(BUILD_DIR "/app/build")
    list(APPEND CMAKE_PREFIX_PATH "/armonik/api")
elseif (WIN32)
    set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/tools/win64/lib/cmake/grpc")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/tools/win64/cmake")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../ArmoniK.Api/packages/cpp/install")
    set(Protobuf_USE_STATIC_LIBS ON)
endif (UNIX)

set(CMAKE_CXX_STANDARD 17)

# make cache variables for install destinations
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_END2END "Build End2End test" OFF)
option(BUILD_TESTS "Build Unit Tests" OFF)

if(BUILD_END2END)
    list(APPEND CMAKE_PREFIX_PATH "/app/install")
    add_subdirectory(ArmoniK.SDK.Client.Test)
else ()
    add_subdirectory(ArmoniK.SDK.Common)
    add_subdirectory(ArmoniK.SDK.Client)
endif ()
