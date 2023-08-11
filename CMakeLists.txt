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
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/tools/win64/lib/cmake/grpc")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/tools/win64/cmake")
    set(Protobuf_USE_STATIC_LIBS ON)
endif ()

set(CMAKE_CXX_STANDARD 17)

# make cache variables for install destinations
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_END2END "Build End2End test" OFF)
option(BUILD_TESTS "Build Unit Tests" OFF)
option(BUILD_WORKER "Build Worker" ON)
option(BUILD_CLIENT "Build Client" ON)
option(BUILD_DYNAMICWORKER "Build DynamicWorker" OFF)

function(setup_options project_name_param)

    file(READ /etc/issue ${PROJECT_NAME}_ETCISSUE_CONTENT)
    string(FIND "${${project_name_param}_ETCISSUE_CONTENT}" "Alpine" IS_ALPINE)

    if(${ARGC} GREATER 1)
        set(extra_param ${ARGV1})
    endif()

    if(MSVC)
        target_compile_options(${project_name_param} PRIVATE /W4)
    else()
        if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
            target_compile_options(${project_name_param} PRIVATE -Wall -Wextra -Wpedantic -fsanitize=undefined,address ${extra_param})
        else ()
            target_compile_options(${project_name_param} PRIVATE -Wall -Wextra -Wpedantic ${extra_param})
        endif()
    endif()

    if(IS_ALPINE EQUAL -1)
        message(STATUS "Not found Alpine distribution " ${IS_ALPINE})
    else()
        message(STATUS "Found Alpine distribution " ${IS_ALPINE})
    endif()

    if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
        target_link_options(${project_name_param} PRIVATE -fsanitize=undefined,address ${extra_param})
    endif()
endfunction()


include(ExternalProject)
ExternalProject_Add(ArmoniK.SDK.Common
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Common"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/ArmoniK.SDK.Common"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=/app/install -DCMAKE_PREFIX_PATH=${ARMONIK_API_DIR}
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/ArmoniK.SDK.Common/libArmoniK.SDK.Common.a
    BUILD_ALWAYS TRUE
    STEP_TARGETS build
)
set(ArmoniK.SDK.Common_BINARY_DIR "${CMAKE_BINARY_DIR}/ArmoniK.SDK.Common/")

execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Common"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Common"
)
ExternalProject_Add_StepTargets(ArmoniK.SDK.Common build)

include(ExternalProject)
ExternalProject_Add(ArmoniK.SDK.Client
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Client"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/ArmoniK.SDK.Client"
    DEPENDS ArmoniK.SDK.Common
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=/app/install -DCMAKE_PREFIX_PATH=${ARMONIK_API_DIR}
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/ArmoniK.SDK.Client/libArmoniK.SDK.Client.a
    BUILD_ALWAYS TRUE
    STEP_TARGETS build
)
set(ArmoniK.SDK.Client_BINARY_DIR "${CMAKE_BINARY_DIR}/ArmoniK.SDK.Client/")

execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Client"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Client"
)

include(ExternalProject)
ExternalProject_Add(ArmoniK.SDK.Worker
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ArmoniK.SDK.Worker"
    BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/ArmoniK.SDK.Worker"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=/app/install -DCMAKE_PREFIX_PATH=${ARMONIK_API_DIR}
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/ArmoniK.SDK.Worker/libArmoniK.SDK.Worker.a
    BUILD_ALWAYS TRUE
    STEP_TARGETS build
)
set(ArmoniK.SDK.Worker_BINARY_DIR "${CMAKE_BINARY_DIR}/ArmoniK.SDK.Worker/")

execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Worker"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../ArmoniK.SDK.Worker"
)

include_directories(/app/install/include)
if(BUILD_DYNAMICWORKER)
    # TODO Move to normal worker build when End2End separated (needs ExternalProject_Add)
    list(APPEND CMAKE_PREFIX_PATH "/app/install")
    add_subdirectory(ArmoniK.SDK.DynamicWorker)
    add_dependencies(ArmoniK.SDK.DynamicWorker ArmoniK.SDK.Common)
else()
    if(BUILD_END2END)
        list(APPEND CMAKE_PREFIX_PATH "/app/install")
        if(BUILD_CLIENT)
            add_subdirectory(ArmoniK.SDK.Client.Test)
            add_dependencies(ArmoniK.SDK.Client.Test ArmoniK.SDK.Client)
            add_dependencies(ArmoniK.SDK.Client.Test ArmoniK.SDK.Common)
        endif()
        if(UNIX AND BUILD_WORKER)
            add_subdirectory(ArmoniK.SDK.Worker.Test)
            add_dependencies(ArmoniK.SDK.Worker.Test ArmoniK.SDK.Worker)
        endif()
    endif ()
endif()
