cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Worker)
set(NAMESPACE ArmoniK::SDK::Worker)
set(version 0.1.0)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

# Trouver les packages requis
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(ArmoniK.Api.Common CONFIG REQUIRED)
find_package(ArmoniK.Api.Worker CONFIG REQUIRED)
find_package(Threads)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_CLIENT_FILES ${HEADER_FILES_DIR}/*.h)

add_library(${PROJECT_NAME} STATIC ${SRC_CLIENT_FILES} ${HEADER_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC protobuf::libprotobuf gRPC::grpc++_unsecure ArmoniK.Api.Worker ArmoniK.Api.Common)

target_compile_definitions(${PROJECT_NAME} PUBLIC SDK_WORKER_NAMESPACE=${NAMESPACE})

target_include_directories(${PROJECT_NAME}
                           PUBLIC
                           "$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
