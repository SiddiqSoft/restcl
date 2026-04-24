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

# ____________________________________
#  Library Definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                            INTERFACE
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

# For std::stop_token and std::jthread on Clang 18 and Clang 19 we need to enable
# the experimental-library flag.
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
CPMAddPackage("gh:SiddiqSoft/SplitUri#3.0.0")
target_link_libraries(${PROJECT_NAME} INTERFACE SplitUri::SplitUri)
CPMAddPackage("gh:SiddiqSoft/AzureCppUtils#3.1.0")
target_link_libraries(${PROJECT_NAME} INTERFACE AzureCppUtils::AzureCppUtils)
CPMAddPackage("gh:SiddiqSoft/string2map#2.5.0")
target_link_libraries(${PROJECT_NAME} INTERFACE string2map::string2map)
CPMAddPackage("gh:SiddiqSoft/RunOnEnd#1.4.0")
target_link_libraries(${PROJECT_NAME} INTERFACE RunOnEnd::RunOnEnd)
CPMAddPackage("gh:SiddiqSoft/asynchrony#2.1.0")
target_link_libraries(${PROJECT_NAME} INTERFACE asynchrony::asynchrony)


# ____________________________________
# Testing
# Available only when building in our repo; controlled by the switch: ${PROJECT_NAME}_BUILD_TESTS = ON
if(${${PROJECT_NAME}_BUILD_TESTS} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" )
    message(STATUS "${PROJECT_NAME} - Asked to build tests.. ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
