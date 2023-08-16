cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME ArmoniK.SDK.Client)
set(NAMESPACE ArmoniK::Sdk::Client)
project(${PROJECT_NAME})
set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(ARMONIK_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/armonik/sdk/client)
set(PROJECT_BUILD_DIR ${BUILD_DIR}/${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

set(VERSION 0.1.0)
include(CMakePackageConfigHelpers)

SET(SOURCES_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(HEADER_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
SET(PRIVATE_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/private")

FILE(GLOB_RECURSE SRC_CLIENT_FILES ${SOURCES_FILES_DIR}/*.cpp)
FILE(GLOB_RECURSE HEADER_CLIENT_FILES ${HEADER_FILES_DIR}/*.h)
FILE(GLOB_RECURSE PRIVATE_HEADERS_FILES ${PRIVATE_HEADERS_DIR}/*.h)

find_package(fmt CONFIG REQUIRED)
find_package(ArmoniK.Api.Client CONFIG REQUIRED)
find_package(ArmoniK.SDK.Common CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC ${SRC_CLIENT_FILES} ${HEADER_CLIENT_FILES} ${PRIVATE_HEADERS_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ArmoniK.Api.Common fmt ArmoniK.Api.Client ArmoniK.SDK.Common)

target_compile_definitions(${PROJECT_NAME} PUBLIC SDK_CLIENT_NAMESPACE=${NAMESPACE})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "$<BUILD_INTERFACE:${HEADER_FILES_DIR}>"
        "$<BUILD_INTERFACE:${PRIVATE_HEADERS_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )

file(READ /etc/issue ${PROJECT_NAME}_ETCISSUE_CONTENT)
string(FIND "${${PROJECT_NAME}_ETCISSUE_CONTENT}" "Alpine" IS_ALPINE)

if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
        if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -fsanitize=undefined,address)
        else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
endif()

if(IS_ALPINE EQUAL -1)
        message(STATUS "Not found Alpine distribution " ${IS_ALPINE})
else()
        message(STATUS "Found Alpine distribution " ${IS_ALPINE})
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG AND IS_ALPINE EQUAL -1)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined,address)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${VERSION})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION 0)
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION 0)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

# generate the version file for the config file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION "${VERSION}"
        COMPATIBILITY AnyNewerVersion
)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${HEADER_FILES_DIR}/ DESTINATION ${ARMONIK_INSTALL_INCLUDE_DIR} FILES_MATCHING PATTERN "*.h")

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        )
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_LIBDIR
        )

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        )

