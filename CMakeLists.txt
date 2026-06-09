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
        HOMEPAGE_URL    "https://github.com/SiddiqSoft/restcl"
        DESCRIPTION     "Focused REST client for modern C++")


# ____________________________________
# Options; define and declare defaults
option(${PROJECT_NAME}_BUILD_TESTS "${PROJECT_NAME} - Build tests. Uncheck for install only runs" OFF)
option(CI_BUILDID "Version of this build" "0.0.0")

set( CMAKE_CXX_STANDARD 23)
set( CMAKE_CXX_STANDARD_REQUIRED On)
set( CMAKE_CXX_EXTENSIONS OFF)
# This will allow cmake aware IDE to pickup on library definitions
set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ____________________________________
#  Library Definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                            INTERFACE
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

# ____________________________________
# Dependencies
if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg]) 
        OR (CMAKE_CXX_COMPILER_ID MATCHES [Gg][Nn][Uu])))
    # Linux/Unix specific
    # Make sure you install libcurl-devel on Linux
    find_package(CURL 8.7 REQUIRED)
    # Note: Do not use include_directories(${CURL_INCLUDE_DIRS}) here.
    # The CURL::libcurl imported target already provides include paths as SYSTEM includes,
    # which avoids conflicts between Homebrew LLVM's C++ headers and the macOS SDK C headers.
    target_link_libraries(${PROJECT_NAME} INTERFACE CURL::libcurl)
elseif((CMAKE_CXX_COMPILER_ID MATCHES [Mm][Ss][Vv][Cc]))
    # Windows specific work
    CPMAddPackage("gh:SiddiqSoft/acw32h#2.7.4")
    target_link_libraries(${PROJECT_NAME} INTERFACE acw32h::acw32h)
endif()

CPMAddPackage("gh:nlohmann/json#v3.12.0")
target_link_libraries(${PROJECT_NAME} INTERFACE nlohmann_json::nlohmann_json)
CPMAddPackage("gh:SiddiqSoft/SplitUri#3.0.3")
target_link_libraries(${PROJECT_NAME} INTERFACE SplitUri::SplitUri)
CPMAddPackage("gh:SiddiqSoft/AzureCppUtils#3.2.5")
target_link_libraries(${PROJECT_NAME} INTERFACE AzureCppUtils::AzureCppUtils)
CPMAddPackage("gh:SiddiqSoft/string2map#2.6.1")
target_link_libraries(${PROJECT_NAME} INTERFACE string2map::string2map)
CPMAddPackage("gh:SiddiqSoft/RunOnEnd#1.4.5")
target_link_libraries(${PROJECT_NAME} INTERFACE RunOnEnd::RunOnEnd)
CPMAddPackage("gh:SiddiqSoft/asynchrony#2.1.3")
target_link_libraries(${PROJECT_NAME} INTERFACE asynchrony::asynchrony)
CPMAddPackage("gh:SiddiqSoft/GenProcessInfo#2.6.1")
target_link_libraries(${PROJECT_NAME} INTERFACE GenProcessInfo::GenProcessInfo)
CPMAddPackage("gh:SiddiqSoft/TimeThis#2.4.1")
target_link_libraries(${PROJECT_NAME} INTERFACE timethis::timethis)

# ____________________________________
# Testing
# Available only when building in our repo; controlled by the switch: ${PROJECT_NAME}_BUILD_TESTS = ON
if(${${PROJECT_NAME}_BUILD_TESTS} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" )
    message(STATUS "${PROJECT_NAME} - Asked to build tests.. ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
