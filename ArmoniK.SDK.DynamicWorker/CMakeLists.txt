cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.DynamicWorker)

project(${PROJECT_NAME})

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

FILE(GLOB_RECURSE SRC_DYNAMICWORKER_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_DYNAMICWORKER_FILES ${HEADER_FILES_DIR}/*.h)

find_package(ArmoniK.Api.Worker REQUIRED CONFIG)
if(NOT TARGET ArmoniK.SDK.Common)
    find_package(ArmoniK.SDK.Common REQUIRED CONFIG)
endif()

find_file(ARMONIK_SDK_HEADER ArmoniKSDKInterface.h HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../ArmoniK.SDK.Worker PATHS ${CMAKE_INSTALL_INCLUDEDIR} PATH_SUFFIXES armonik/sdk/worker include/armonik/sdk/worker REQUIRED NO_CACHE)
message(STATUS "Found ArmoniKSDKInterface.h at location ${ARMONIK_SDK_HEADER}")
get_filename_component(ARMONIK_SDK_HEADER_DIR ${ARMONIK_SDK_HEADER} DIRECTORY)
# Include directory of the worker headers (<armonik/sdk/worker/...>), three levels above ArmoniKSDKInterface.h
get_filename_component(ARMONIK_SDK_INCLUDE_DIR "${ARMONIK_SDK_HEADER_DIR}/../../.." ABSOLUTE)

if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endif()

add_executable(${PROJECT_NAME} ${SRC_DYNAMICWORKER_FILES} ${HEADER_DYNAMICWORKER_FILES})

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})
split_version(${VERSION})
setup_lib_version(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.Api.Worker ArmoniK.SDK.Common ArmoniK.Api.Common nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
        "$<BUILD_INTERFACE:${ARMONIK_SDK_HEADER_DIR}>"
        "$<BUILD_INTERFACE:${ARMONIK_SDK_INCLUDE_DIR}>"
        )

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/appsettings.json
        ${CMAKE_CURRENT_BINARY_DIR}/appsettings.json)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES appsettings.json DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
