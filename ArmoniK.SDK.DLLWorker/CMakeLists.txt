cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.DLLWorker)
set(NAMESPACE ArmoniK::SDK::DLLWorker)
set(version 0.1.0)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_CLIENT_FILES ${SOURCES_FILES_DIR}/*.h)

find_package(ArmoniK.SDK.Worker CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES} ${HEADER_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.SDK.Common ArmoniK.SDK.Worker)

target_compile_definitions(${PROJECT_NAME} PUBLIC SDK_DLLWORKER_NAMESPACE=${NAMESPACE})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${SOURCES_FILES_DIR}"
        )