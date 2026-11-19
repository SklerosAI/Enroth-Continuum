cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

if(OE_BUILD_TESTS)
    set(TESTING_UNIT_SOURCES
            MM6TestData.cpp
            UnitTest.cpp)
    set(TESTING_UNIT_HEADERS
            MM6TestData.h
            UnitTest.h)

    add_library(testing_unit ${TESTING_UNIT_SOURCES} ${TESTING_UNIT_HEADERS})
    target_link_libraries(testing_unit PUBLIC testing_extensions library_lod library_lod_formats library_filesystem_lowercase
            library_filesystem_native GTest::gtest GTest::gmock)

    target_check_style(testing_unit)
endif()
