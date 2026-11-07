cmake_minimum_required(VERSION 3.27)

# Can't have git revision as a global #define, thus this lib.

set(LIBRARY_BUILDINFO_SOURCES
        BuildInfo.cpp)

set(LIBRARY_BUILDINFO_HEADERS
        BuildInfo.h)

set(GIT_REVISION "${PROJECT_VERSION}")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE DETECTED_GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE GIT_REVISION_STATUS)
    if(GIT_REVISION_STATUS EQUAL 0 AND NOT "${DETECTED_GIT_REVISION}" STREQUAL "")
        set(GIT_REVISION "${DETECTED_GIT_REVISION}")
    endif()
endif()

string(TIMESTAMP BUILD_TIME UTC)

add_library(library_buildinfo ${LIBRARY_BUILDINFO_SOURCES} ${LIBRARY_BUILDINFO_HEADERS})
target_compile_definitions(library_buildinfo PRIVATE
        MM_GIT_REVISION="${GIT_REVISION}"
        MM_BUILD_TIME="${BUILD_TIME}")
target_check_style(library_buildinfo)
