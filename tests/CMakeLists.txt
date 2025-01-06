if(${${PROJECT_NAME}_BUILD_TESTS})
    set(TESTPROJ ${PROJECT_NAME}_tests)

    set( CMAKE_CXX_STANDARD 23)
    set( CMAKE_CXX_STANDARD_REQUIRED On)
    set( CMAKE_CXX_EXTENSIONS Off)
    
    add_executable(${TESTPROJ})

    target_compile_features(${TESTPROJ} PRIVATE cxx_std_23)
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest> )
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library> )

    # ASAN and Coverage only for Debug builds on Linux
    if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg]) 
        OR (CMAKE_CXX_COMPILER_ID MATCHES [Gg][Nn][Uu]))
       AND (CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg]))
        message(STATUS "Using sanitizers for leak and address on ${CMAKE_CXX_COMPILER_ID}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak")
        target_link_libraries(${TESTPROJ} PRIVATE -fsanitize=address,leak)
        message(STATUS "Enable instrumentation for code coverage")
        target_compile_options(${TESTPROJ} PRIVATE -coverage -O0 -g)
        target_link_options(${TESTPROJ} PRIVATE -coverage)
    endif()

    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        add_compile_definitions(${TESTPROJ} PRIVATE DEBUG=1 _DEBUG=1)
    endif()

    # Dependencies
    cpmaddpackage("gh:google/googletest#v1.15.2")

    target_sources( ${TESTPROJ}
                    PRIVATE
                    ${PROJECT_SOURCE_DIR}/tests/test_restcl.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_basics.cpp
                    ${PROJECT_SOURCE_DIR}/tests/test_http2json.cpp
                    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${PROJECT_SOURCE_DIR}/tests/test_libcurl_helpers.cpp>
                    #$<$<CXX_COMPILER_ID:Clang>:${PROJECT_SOURCE_DIR}/tests/test_libcurl_helpers.cpp>
                    ${PROJECT_SOURCE_DIR}/tests/test_openssl_helpers.cpp)
    # Link dependencies..
    target_link_libraries(${TESTPROJ} PRIVATE
            GTest::gtest_main
            nlohmann_json::nlohmann_json
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:CURL::libcurl>
            $<$<CXX_COMPILER_ID:MSVC>:acw32h::acw32h>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:OpenSSL::SSL>
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:OpenSSL::Crypto>
            RunOnEnd::RunOnEnd
            SplitUri::SplitUri
            string2map::string2map
            asynchrony::asynchrony
            AzureCppUtils::AzureCppUtils
            StringHelpers::StringHelpers)

    include(GoogleTest)
    gtest_discover_tests(${TESTPROJ} XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests/results")

    include(CTest)
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
endif()
