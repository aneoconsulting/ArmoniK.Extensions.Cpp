cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Worker.UnitTest)

project(${PROJECT_NAME})

# Tests of the worker side that need no cluster: TaskContext and the C interface of armonik_process, and, against an
# in-process agent, ConventionExecutor when ArmoniK.Api.Worker is found and the DynamicWorker when it is built too
# (BUILD_DYNAMICWORKER=ON)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(nlohmann_json REQUIRED)
find_package(ArmoniK.Api.Common CONFIG REQUIRED)
find_package(ArmoniK.Api.Worker CONFIG QUIET)

set(SRC_FILES ${SOURCES_FILES_DIR}/TaskContextTest.cpp)
if(TARGET ArmoniK.Api.Worker)
	list(APPEND SRC_FILES ${SOURCES_FILES_DIR}/ConventionExecutorTest.cpp)
	if(TARGET ArmoniK.SDK.DynamicWorker.Core)
		list(APPEND SRC_FILES ${SOURCES_FILES_DIR}/DynamicWorkerTest.cpp)
	endif()
endif()

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.SDK.Worker ArmoniK.SDK.Common ArmoniK.Api.Common
		nlohmann_json::nlohmann_json GTest::gtest_main ${CMAKE_DL_LIBS})
if(TARGET ArmoniK.Api.Worker)
	target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.Api.Worker)
	if(TARGET ArmoniK.SDK.DynamicWorker.Core)
		target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.SDK.DynamicWorker.Core)
	endif()
endif()
target_include_directories(${PROJECT_NAME} PRIVATE "${HEADER_FILES_DIR}")

# Library with its own armonik_call, loaded by the tests as the DynamicWorker loads libraries
add_library(ArmoniK.SDK.Worker.CustomCallTest SHARED ${CMAKE_CURRENT_SOURCE_DIR}/libs/CustomCallLibrary.cpp)
target_link_libraries(ArmoniK.SDK.Worker.CustomCallTest PRIVATE "-Wl,--whole-archive" ArmoniK.SDK.Worker "-Wl,--no-whole-archive")
add_dependencies(${PROJECT_NAME} ArmoniK.SDK.Worker.CustomCallTest)
target_compile_definitions(${PROJECT_NAME} PRIVATE "CUSTOM_CALL_LIBRARY=\"$<TARGET_FILE:ArmoniK.SDK.Worker.CustomCallTest>\"")

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 OLD)
endif()

# gTest support, shared with ArmoniK.SDK.Client.Test when both are built
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
