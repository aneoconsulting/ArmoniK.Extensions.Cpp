cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client.Examples.Hello)

project(${PROJECT_NAME})

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_CLIENT_FILES ${HEADER_FILES_DIR}/*.h)

find_package(ArmoniK.Api.Client CONFIG REQUIRED)

if(NOT TARGET ArmoniK.SDK.Common)
	find_package(ArmoniK.SDK.Common CONFIG REQUIRED)
endif()
if(NOT TARGET ArmoniK.SDK.Client)
	find_package(ArmoniK.SDK.Client CONFIG REQUIRED)
endif()

add_executable(${PROJECT_NAME} ${SRC_CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ArmoniK.Api.Common ArmoniK.Api.Client ArmoniK.SDK.Common ArmoniK.SDK.Client)
target_include_directories(${PROJECT_NAME}
		PUBLIC
		"$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils.cmake)
setup_options(${PROJECT_NAME})
split_version(${VERSION})
setup_lib_version(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 OLD)
endif()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})
