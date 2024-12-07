if(${${PROJECT_NAME}_BUILD_TESTS})
    set(TESTPROJ ${PROJECT_NAME}_tests)

    add_executable(${TESTPROJ})
    target_compile_features(${TESTPROJ} PRIVATE cxx_std_20)
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:MSVC>:/std:c++20> )
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library> )

    target_sources( ${TESTPROJ}
                    PRIVATE
                    ${PROJECT_SOURCE_DIR}/tests/test.cpp)

    # Dependencies
    cpmaddpackage("gh:google/googletest#v1.15.2")
    target_link_libraries(${TESTPROJ} PRIVATE GTest::gtest_main)
    CPMAddPackage("gh:nlohmann/json#v3.11.3")
    target_link_libraries(${TESTPROJ} PRIVATE nlohmann_json::nlohmann_json)
    CPMAddPackage("gh:SiddiqSoft/SplitUri#main")
    target_link_libraries(${TESTPROJ} PRIVATE SplitUri::SplitUri)
    CPMAddPackage("gh:SiddiqSoft/AzureCppUtils#main")
    target_link_libraries(${TESTPROJ} PRIVATE AzureCppUtils::AzureCppUtils)
    CPMAddPackage("gh:SiddiqSoft/acw32h#main")
    target_link_libraries(${TESTPROJ} PRIVATE acw32h::acw32h)
    CPMAddPackage("gh:SiddiqSoft/string2map#main")
    target_link_libraries(${TESTPROJ} PRIVATE string2map::string2map)
    CPMAddPackage("gh:SiddiqSoft/asynchrony#main")
    target_link_libraries(${TESTPROJ} PRIVATE asynchrony::asynchrony)
        
    include(GoogleTest)

    gtest_discover_tests(${TESTPROJ} XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests/results")

    include(CTest)
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
endif()
