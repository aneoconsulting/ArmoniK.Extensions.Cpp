cmake_minimum_required(VERSION 3.22)

set(VERSION 0.1.0)

set(SOLUTION_NAME ArmoniK.SDK)
project(${SOLUTION_NAME} C CXX)

if(NOT DEFINED BUILD_DIR)
    set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")
endif()
if(NOT DEFINED ARMONIK_API_DIR)
    if(UNIX)
        set(ARMONIK_API_DIR "/armonik/api")
    else()
        set(ARMONIK_API_DIR ${CMAKE_SOURCE_DIR}/../ArmoniK.Api/packages/cpp/out/install/x64-Debug)
    endif()
endif()
list(APPEND CMAKE_PREFIX_PATH ${ARMONIK_API_DIR})
if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../ArmoniK.Api/packages/cpp/tools/win64/lib/cmake/grpc")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../ArmoniK.Api/packages/cpp/tools/win64/cmake")
    set(INSTALL_SDK_DIR ${CMAKE_SOURCE_DIR}/install)
    set(Protobuf_USE_STATIC_LIBS ON)
else()
    # set(INSTALL_SDK_DIR "/app/install")
    if(NOT DEFINED INSTALL_SDK_DIR)
        set(INSTALL_SDK_DIR "${CMAKE_SOURCE_DIR}/install")
    endif()
endif ()
list(APPEND CMAKE_PREFIX_PATH ${INSTALL_SDK_DIR})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH}")

set(CMAKE_CXX_STANDARD 17)

# make cache variables for install destinations
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_END2END "Build End2End test" OFF)
option(BUILD_COMMON "Build Common" ON)
option(BUILD_WORKER "Build Worker" ON)
option(BUILD_CLIENT "Build Client" ON)
option(BUILD_DYNAMICWORKER "Build DynamicWorker" OFF)
option(BUILD_WORKERTEST "Build Worker Test" OFF)
option(BUILD_SDK "Build SDK" ON)

include(ExternalProject)

add_custom_target(${SOLUTION_NAME} ALL)

if(BUILD_SDK)
    ExternalProject_Add(ArmoniK.SDK.Common
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Common"
        BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.Common"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
        CONFIGURE_HANDLED_BY_BUILD TRUE
        BUILD_ALWAYS TRUE
        INSTALL_DIR ${INSTALL_SDK_DIR}
        LIST_SEPARATOR |
    )

    add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.Common)

    if(BUILD_WORKER)
        ExternalProject_Add(ArmoniK.SDK.Worker
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Worker"
            BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.Worker"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
            CONFIGURE_HANDLED_BY_BUILD TRUE
            BUILD_ALWAYS TRUE
            INSTALL_DIR ${INSTALL_SDK_DIR}
            LIST_SEPARATOR |
        )
        add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.Worker)
    endif()

    if(BUILD_CLIENT)
        ExternalProject_Add(ArmoniK.SDK.Client
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Client"
            BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.Client"
            DEPENDS ArmoniK.SDK.Common
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
            CONFIGURE_HANDLED_BY_BUILD TRUE
            BUILD_ALWAYS TRUE
            INSTALL_DIR ${INSTALL_SDK_DIR}
            LIST_SEPARATOR |
        )
        add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.Client)
    endif()
endif()

if(BUILD_DYNAMICWORKER)
    If(NOT BUILD_WORKER)
        find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
        find_package(ArmoniK.SDK.Worker CONFIG REQUIRED)
    endif()
    ExternalProject_Add(ArmoniK.SDK.DynamicWorker
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.DynamicWorker"
        BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.DynamicWorker"
        DEPENDS ArmoniK.SDK.Common ArmoniK.SDK.Worker
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
        CONFIGURE_HANDLED_BY_BUILD TRUE
        BUILD_ALWAYS TRUE
        INSTALL_DIR ${INSTALL_SDK_DIR}
        LIST_SEPARATOR |
    )
    add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.DynamicWorker)
endif()

if(BUILD_END2END)
    if(BUILD_CLIENT)
        if(NOT BUILD_SDK)
            find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
            find_package(ArmoniK.SDK.Client CONFIG REQUIRED)
        endif()
        ExternalProject_Add(ArmoniK.SDK.Client.Test
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Client.Test"
            BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.Client.Test"
            DEPENDS ArmoniK.SDK.Common ArmoniK.SDK.Client
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
            CONFIGURE_HANDLED_BY_BUILD TRUE
            BUILD_ALWAYS TRUE
            INSTALL_DIR ${INSTALL_SDK_DIR}
            LIST_SEPARATOR |
        )
        add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.Client.Test)
    endif()

    if(UNIX AND BUILD_WORKER)
        if(NOT BUILD_SDK)
            find_package(ArmoniK.SDK.Worker CONFIG REQUIRED)
        endif()
        ExternalProject_Add(ArmoniK.SDK.Worker.Test
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Worker.Test"
            BINARY_DIR "${BUILD_DIR}/ArmoniK.SDK.Worker.Test"
            DEPENDS ArmoniK.SDK.Worker
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL_SDK_DIR} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP} -DVERSION=${VERSION}
            CONFIGURE_HANDLED_BY_BUILD TRUE
            BUILD_ALWAYS TRUE
            INSTALL_DIR ${INSTALL_SDK_DIR}
            LIST_SEPARATOR |
        )
        add_dependencies(${SOLUTION_NAME} ArmoniK.SDK.Worker.Test)
    endif()
endif ()


install(CODE "")
