cmake_minimum_required(VERSION 3.22)

set(version 0.1.0)

set(SOLUTION_NAME ArmoniK.SDK)
project(${SOLUTION_NAME} C CXX)

set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_PREFIX_PATH "/armonik/api")
set(ARMONIK_COMMON_INCLUDE_DIR "/armonik/api/include/armonik/common")
set(ARMONIK_CLIENT_INCLUDE_DIR "/armonik/api/include/armonik/client")
set(ARMONIK_WORKER_INCLUDE_DIR "/armonik/api/include/armonik/worker")

add_subdirectory(ArmoniK.SDK.Client)