cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client.Test)

project(${PROJECT_NAME})

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_CLIENT_FILES ${HEADER_FILES_DIR}/*.h)

# The worker task context is tested here when the worker library is built with the client
if(NOT TARGET ArmoniK.SDK.Worker)
	list(REMOVE_ITEM SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/TaskContextTest.cpp)
endif()

find_package(ArmoniK.Api.Client CONFIG REQUIRED)

if(NOT TARGET ArmoniK.SDK.Common)
	find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
endif()
if(NOT TARGET ArmoniK.SDK.Client)
	find_package(ArmoniK.SDK.Client CONFIG REQUIRED)
endif()

find_package(nlohmann_json REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ArmoniK.Api.Common ArmoniK.Api.Client ArmoniK.SDK.Common ArmoniK.SDK.Client)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest_main nlohmann_json::nlohmann_json)
if(TARGET ArmoniK.SDK.Worker)
	target_link_libraries(${PROJECT_NAME} PRIVATE ArmoniK.SDK.Worker ${CMAKE_DL_LIBS})

	# Library with its own armonik_call, loaded by the tests as the DynamicWorker loads libraries
	add_library(ArmoniK.SDK.Worker.CustomCallTest SHARED ${CMAKE_CURRENT_SOURCE_DIR}/libs/CustomCallLibrary.cpp)
	target_link_libraries(ArmoniK.SDK.Worker.CustomCallTest PRIVATE "-Wl,--whole-archive" ArmoniK.SDK.Worker "-Wl,--no-whole-archive")
	add_dependencies(${PROJECT_NAME} ArmoniK.SDK.Worker.CustomCallTest)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "CUSTOM_CALL_LIBRARY=\"$<TARGET_FILE:ArmoniK.SDK.Worker.CustomCallTest>\"")
endif()
target_include_directories(${PROJECT_NAME}
		PUBLIC
		"$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../Utils.cmake)
setup_options(${PROJECT_NAME})
split_version(${VERSION})
setup_lib_version(${PROJECT_NAME})

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
