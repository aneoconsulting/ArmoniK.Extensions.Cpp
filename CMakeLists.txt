cmake_minimum_required(VERSION 3.22)

set(version 0.1.0)

set(SOLUTION_NAME ArmoniK.SDK)
project(${SOLUTION_NAME} C CXX)

set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_PREFIX_PATH "/armonik/api")

add_subdirectory(ArmoniK.SDK.Common)
add_subdirectory(ArmoniK.SDK.Client)
add_subdirectory(ArmoniK.SDK.Client.Test)
