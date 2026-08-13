#   Copyright 2026 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
#   When building from our CI server, the following options must be set:
#   - aarp_BUILD_TESTS=ON
#   - CI_BUILDID must be set to the gitversion
#   The file project-base.json contains configuration elements
# 
cmake_minimum_required(VERSION 3.31)

# Version detection must precede project() declaration
# Figures out ${CURRENT_PROJECT_VERSION} and brings in CPM module
if(WIN32)
    if(DEFINED ENV{LOCALAPPDATA})
        set(CPM_SOURCE_CACHE "$ENV{LOCALAPPDATA}/CPM/.cpmcache")
    elseif(DEFINED ENV{USERPROFILE})
        set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/AppData/Local/CPM/.cpmcache")
    else()
        set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/.cpmcache")
    endif()
else()
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/.cpmcache")
endif()
set(CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE} CACHE PATH "CPM source cache")
include(../../pack/CMakeCommonHelpers.cmake)

# Project metadata
# defaults are in the project-base.json file
project(examples_cosmosprobes
        DESCRIPTION     "Cosmos Probes Example"
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        HOMEPAGE_URL    "%%myurl%%")

# Build options

# Cleanup the previous gcda files..
# Find all .gcda files recursively in the binary/output directory
file(GLOB_RECURSE GCDA_FILES "${CMAKE_BINARY_DIR}/*.gcda")

# Remove the found files if the list is not empty
if(GCDA_FILES)
    file(REMOVE ${GCDA_FILES})
    message(STATUS "Removed old .gcda files: ${GCDA_FILES}")
endif()

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
target_compile_options( ${PROJECT_NAME}
                        PRIVATE
                        $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest> )

# Check if mold is available on the system
find_program(MOLD_LINKER mold)

if(MOLD_LINKER)
    message(STATUS "  >> Found mold linker, using it for build.")
    # Apply mold as the linker for executables and shared libraries
    add_link_options("-fuse-ld=mold")
else()
    message(STATUS "  >> mold not found, falling back to default linker.")
endif()

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE AND CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
    # This runs the analyzer with all default checks enabled
    # We skip anything that is not in our project.
    message(STATUS "  >> Using clang-tidy checks=*")
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE} 
        "-checks=*"
        "--header-filter=^(${CMAKE_CURRENT_SOURCE_DIR}/|${PROJECT_SOURCE_DIR}/include/)")
endif()

# UndefinedBehaviorSanitizer is not available on Windows
if( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
    message(STATUS "  >> UBSAN is not supported on Windows/MSVC, skipping..")
    message(STATUS "  >> Coverage flags is not supported on Windows/MSVC, skipping..")
    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        set(DEBUG_COMPILE_FLAGS /Zi /Od)
        set(DEBUG_LINK_FLAGS /DEBUG:FULL)
    endif()
else()
    set(ASAN_FLAGS "-fsanitize=address")
    set(UBSAN_FLAGS "-fsanitize=undefined")
    # Coverage Flags is not available on Windows
    #set(COVERAGE_FLAGS "-coverage")
    set(OPTIMIZE_FLAGS "-O0")
    set(OTHER_FLAGS "-g")
    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        set(DEBUG_COMPILE_FLAGS -g3 -O0)
    endif()
endif()

# LeakSanitizer is not available on Windows and Darwin/macOS with AppleClang
if( CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_SYSTEM_NAME STREQUAL "Windows" )
    message(STATUS "  >> LSAN is not supported on Windows/MSVC and AppleClang, skipping..")
else()
    set(LSAN_FLAGS "-fsanitize=leak")
endif()

message(STATUS "  >> Enable compile options for ${PROJECT_NAME} -- ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${COVERAGE_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE  ${DEBUG_COMPILE_FLAGS}  ${COVERAGE_FLAGS} ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS})
target_link_options(${PROJECT_NAME} PRIVATE ${DEBUG_LINK_FLAGS}  ${COVERAGE_FLAGS} ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS})

if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
    message(STATUS "  >> Setting DEBUG macro..")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1 _DEBUG=1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_TRACE=1)
endif()

# Our tests depend on this preprocessor definition during CI/test mode to enable
# access to protected members.
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_TESTING_MODE=1)

# On macOS with Homebrew LLVM, the CURL::libcurl imported target adds the Xcode SDK
# include path (-isystem .../MacOSX.sdk/usr/include) which conflicts with LLVM's own
# libc++ headers. The SDK's C headers get found before LLVM's stddef.h wrapper,
# causing "unknown type name 'size_t'" errors.
# Fix: Tell CMake that the SDK include path is an implicit system directory so it
# won't be emitted as an explicit -isystem flag on the command line.
if(CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg] AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE _sdk_path
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_sdk_path AND EXISTS "${_sdk_path}/usr/include")
        list(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${_sdk_path}/usr/include")
    endif()
endif()

if(GCDA_FILES)
    file(REMOVE ${GCDA_FILES})
    message(STATUS "Removed old .gcda files: ${GCDA_FILES}")
endif()

# Dependencies
CPMAddPackage("gh:google/googletest#v1.17.0")
CPMAddPackage("gh:nlohmann/json#v3.12.0")

CPMAddPackage("gh:siddiqsoft/restcl#dev")

target_sources(${PROJECT_NAME}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src/probes.cpp
)

# Link dependencies..
target_link_libraries(${PROJECT_NAME} PRIVATE
        nlohmann_json::nlohmann_json
        restcl::restcl
        )

if(GCDA_FILES)
    file(REMOVE ${GCDA_FILES})
    message(STATUS "Removed old .gcda files: ${GCDA_FILES}")
endif()
