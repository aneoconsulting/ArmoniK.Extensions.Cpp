cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client.Test)
set(NAMESPACE ArmoniK::SDK::Client::Test)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)

find_package(ArmoniK.SDK.Client CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.SDK.Common ArmoniK.SDK.Client)

target_compile_definitions(${PROJECT_NAME} PUBLIC SDK_CLIENT_TEST_NAMESPACE=${NAMESPACE})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${SOURCES_FILES_DIR}"
        )

FILE(COPY appsettings.json DESTINATION "${PROJECT_BINARY_DIR}")
