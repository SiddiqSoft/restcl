# download CPM.cmake..
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake ${CMAKE_CURRENT_SOURCE_DIR}/pack/CPM.cmake)
# import the helper into our process..
include(pack/CPM.cmake)

# ____________________________
# Detect Build Id and Versions
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Detect and normalize the version
    if(DEFINED CI_BUILDID)
        message(STATUS "Using the CI supplied buildid: CURRENT_PROJECT_VERSION --> ${CI_BUILDID}")

        # Check if we have a last fragment.
        # When building a non release we end up with fragmented/temporary gitversion
        # 1.2.3-branch-name-5.9+99
        # where the `99` at the end indicates the commit count on this branch.
        set(VERSION_LAST "")
        string(REGEX MATCH "[\+]\+([0-9]+)$" VERSION_END_MATCH ${CI_BUILDID})
        message(STATUS "Match count: ${CMAKE_MATCH_COUNT}")
        if(CMAKE_MATCH_COUNT EQUAL 1)
            set(VERSION_LAST         "${CMAKE_MATCH_1}")
        endif()

        # Match the first full version
        string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.*([0-9]+)*" VERSION_MATCH ${CI_BUILDID})
        message(STATUS "Match count: ${CMAKE_MATCH_COUNT}")
        if(CMAKE_MATCH_COUNT EQUAL 3)
            set(VERSION_MAJOR         "${CMAKE_MATCH_1}")
            set(VERSION_MINOR         "${CMAKE_MATCH_2}")
            set(VERSION_PATCH         "${CMAKE_MATCH_3}")
            set(CURRENT_PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
        elseif(CMAKE_MATCH_COUNT EQUAL 4)
            set(VERSION_MAJOR         "${CMAKE_MATCH_1}")
            set(VERSION_MINOR         "${CMAKE_MATCH_2}")
            set(VERSION_PATCH         "${CMAKE_MATCH_3}")
            set(CURRENT_PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${CMAKE_MATCH_4})
        endif()
        if(NOT (VERSION_LAST STREQUAL ""))
            string(APPEND CURRENT_PROJECT_VERSION ".${VERSION_LAST}")
        endif()
        message(STATUS "Parsed out the CI_BUILDID  --> CURRENT_PROJECT_VERSION:${CURRENT_PROJECT_VERSION}")
    else()
        set(CURRENT_PROJECT_VERSION "0.0.0.0")
        message(STATUS "Using the dev-only version --> CURRENT_PROJECT_VERSION:${CURRENT_PROJECT_VERSION}")
    endif()
endif()
