cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client.Test)

project(${PROJECT_NAME})

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)

if(NOT TARGET ArmoniK.SDK.Common)
	find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
endif()
if(NOT TARGET ArmoniK.SDK.Client)
	find_package(ArmoniK.SDK.Client CONFIG REQUIRED)
endif()

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ArmoniK.SDK.Common ArmoniK.SDK.Client)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main)

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})
split_version(${VERSION})
setup_lib_version(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

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
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})
