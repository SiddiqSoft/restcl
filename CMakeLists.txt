#   SiddiqSoft CMakeLists 
#   Copyright 2024 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
#   When building from our CI server, the following options must be set
#   BUILD_TESTS ON
#   CI_BUILDID must be set to the gitversion
# 
cmake_minimum_required(VERSION 3.29)

# _________________
# Version detection; must preceed the declaration of the actual project()
# Figures out the value ${CURRENT_PROJECT_VERSION} and therefore the ${PROJECT_VERSION}
# Brings in the CPM module into pack/CPM.make
include(pack/CMakeCommonHelpers.cmake)

# ______________________
# Project Metadata block
project(restcl
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        HOMEPAGE_URL    "%%myurl%%"
        DESCRIPTION     "Focused REST client for modern C++")


# ____________________________________
# Options; define and declare defaults
option(${PROJECT_NAME}_BUILD_TESTS "${PROJECT_NAME} - Build tests. Uncheck for install only runs" OFF)

# ____________________________________
#  Library Definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                            INTERFACE
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# For std::stop_token and std::jthread on Clang 18 and Clang 19 we need to enable
# the experimental-library flag.
message(STATUS "Compiler for ${PROJECT_NAME} `${CMAKE_CXX_COMPILER_ID}`")
target_compile_options( ${PROJECT_NAME}
                        INTERFACE
                        $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library> )

set( CMAKE_CXX_STANDARD 23)
set( CMAKE_CXX_STANDARD_REQUIRED On)
set( CMAKE_CXX_EXTENSIONS Off)
                        
# ____________________________________
# Dependencies
if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg]) 
        OR (CMAKE_CXX_COMPILER_ID MATCHES [Gg][Nn][Uu])))
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} INTERFACE CURL::libcurl)

    find_package(OpenSSL 3.0 REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif()

CPMAddPackage("gh:nlohmann/json#v3.11.3")
target_link_libraries(${PROJECT_NAME} INTERFACE nlohmann_json::nlohmann_json)
CPMAddPackage("gh:SiddiqSoft/SplitUri#2.1.0")
target_link_libraries(${PROJECT_NAME} INTERFACE SplitUri::SplitUri)
CPMAddPackage("gh:SiddiqSoft/AzureCppUtils#3.0.1")
target_link_libraries(${PROJECT_NAME} INTERFACE AzureCppUtils::AzureCppUtils)
CPMAddPackage("gh:SiddiqSoft/acw32h#2.7.4")
target_link_libraries(${PROJECT_NAME} INTERFACE acw32h::acw32h)
CPMAddPackage("gh:SiddiqSoft/string2map#2.4.2")
target_link_libraries(${PROJECT_NAME} INTERFACE string2map::string2map)
CPMAddPackage("gh:SiddiqSoft/asynchrony#1.8.0")
target_link_libraries(${PROJECT_NAME} INTERFACE asynchrony::asynchrony)


# ____________________________________
# Testing
# Available only when building in our repo; controlled by the switch: ${PROJECT_NAME}_BUILD_TESTS = ON
if(${${PROJECT_NAME}_BUILD_TESTS} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" )
    message(STATUS "${PROJECT_NAME} - Asked to build tests.. ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
