if(${${PROJECT_NAME}_BUILD_TESTS})
    set(TESTPROJ ${PROJECT_NAME}_tests)

    add_executable(${TESTPROJ})

    target_compile_features(${TESTPROJ} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
    target_compile_options( ${TESTPROJ}
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
        set(COVERAGE_FLAGS "-coverage")
        set(OPTIMIZE_FLAGS "-O0")
        set(OTHER_FLAGS "-g")
        if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
            set(DEBUG_COMPILE_FLAGS -g3)
        endif()
    endif()

    # LeakSanitizer is not available on Windows and Darwin/macOS with AppleClang
    if( CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_SYSTEM_NAME STREQUAL "Windows" )
        message(STATUS "  >> LSAN is not supported on Windows/MSVC and AppleClang, skipping..")
    else()
        message(STATUS "  >> Setting LSAN for ${CMAKE_CXX_COMPILER_ID} / ${CMAKE_SYSTEM_NAME}")
        set(LSAN_FLAGS "-fsanitize=leak")
    endif()

    message(STATUS "  >> Enable compile options for ${TESTPROJ} -- ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${COVERAGE_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS}")
    target_compile_options(${TESTPROJ} PRIVATE  ${DEBUG_COMPILE_FLAGS}  ${COVERAGE_FLAGS} ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS})
    target_link_options(${TESTPROJ} PRIVATE ${DEBUG_LINK_FLAGS}  ${COVERAGE_FLAGS} ${OPTIMIZE_FLAGS} ${OTHER_FLAGS} ${ASAN_FLAGS} ${LSAN_FLAGS} ${UBSAN_FLAGS})

    target_compile_definitions(${TESTPROJ} PRIVATE RESTCL_ENABLE_TEST_HOOKS=1)

    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        message(STATUS "  >> Setting DEBUG macro..")
        target_compile_definitions(${TESTPROJ} PRIVATE DEBUG=1 _DEBUG=1)
        if(${PROJECT_NAME}_DEBUG_TRACE)
            message(STATUS "  >> Setting DEBUG_TRACE macro..")
            target_compile_definitions(${TESTPROJ} PRIVATE DEBUG_TRACE=1)
        endif()
    endif()

    # Dependencies
    CPMAddPackage("gh:google/googletest#v1.17.0")

    # Sources
    target_sources( ${TESTPROJ}
                    PRIVATE
                    ${PROJECT_SOURCE_DIR}/tests/test_serializers.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_validation.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_postbin.cpp
                    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${PROJECT_SOURCE_DIR}/tests/test_libcurl_helpers.cpp>
                    ${PROJECT_SOURCE_DIR}/tests/test_restcl.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_edge_cases.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_mock_and_coverage.cpp)
    # Link dependencies..
    target_link_libraries(${TESTPROJ} PRIVATE
            GTest::gtest_main
            nlohmann_json::nlohmann_json
            ctre::ctre
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:CURL::libcurl>
            $<$<CXX_COMPILER_ID:MSVC>:acw32h::acw32h>
            RunOnEnd::RunOnEnd
            SplitUri::SplitUri
            arrp::arrp
            string2map::string2map
            asynchrony::asynchrony
            RWLEnvelope::RWLEnvelope
            AzureCppUtils::AzureCppUtils
            StringHelpers::StringHelpers)

    include(GoogleTest)
    gtest_discover_tests(${TESTPROJ} XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests/results")
    
    include(CTest)
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
endif()
