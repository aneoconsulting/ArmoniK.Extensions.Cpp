cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client.Test)
set(NAMESPACE ArmoniK::Sdk::Client::Test)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)

find_package(fmt CONFIG REQUIRED)
find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
find_package(ArmoniK.SDK.Client CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC fmt ArmoniK.SDK.Common ArmoniK.SDK.Client GTest::gtest_main)

target_compile_definitions(${PROJECT_NAME} PUBLIC SDK_CLIENT_TEST_NAMESPACE=${NAMESPACE})

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${SOURCES_FILES_DIR}"
        "$<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )

FILE(COPY appsettings.json DESTINATION "${PROJECT_BINARY_DIR}")

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 OLD)
endif()

# gTest support
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${VERSION})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION 0)
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION 0)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})
