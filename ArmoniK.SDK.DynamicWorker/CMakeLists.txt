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

find_file(ARMONIK_SDK_HEADER ArmoniKSDKInterface.h PATHS ${CMAKE_INSTALL_INCLUDEDIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ArmoniK.SDK.Worker PATH_SUFFIXES armonik/sdk/worker include/armonik/sdk/worker REQUIRED NO_CACHE)
message(STATUS "Found ArmoniKSDKInterface.h at location ${ARMONIK_SDK_HEADER}")
get_filename_component(ARMONIK_SDK_HEADER_DIR ${ARMONIK_SDK_HEADER} DIRECTORY)

add_executable(${PROJECT_NAME} ${SRC_DYNAMICWORKER_FILES} ${HEADER_DYNAMICWORKER_FILES})

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})
split_version(${VERSION})
setup_lib_version(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.Api.Worker ArmoniK.Api.Common ArmoniK.SDK.Common)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
        "$<BUILD_INTERFACE:${ARMONIK_SDK_HEADER_DIR}>"
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
