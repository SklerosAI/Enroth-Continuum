cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_library(engine_mm6_save_container STATIC MM6SaveContainer.cpp MM6SaveContainer.h)
target_check_style(engine_mm6_save_container)

add_library(engine_mm6_outdoor_wire STATIC MM6OutdoorWire.cpp MM6OutdoorWire.h)
target_check_style(engine_mm6_outdoor_wire)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_save_container OBJECT Tests/MM6SaveContainer_ut.cpp)
    target_link_libraries(test_engine_mm6_save_container PUBLIC testing_unit engine_mm6_save_container)
    target_check_style(test_engine_mm6_save_container)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_container)

    add_library(test_engine_mm6_outdoor_wire OBJECT Tests/MM6OutdoorWire_ut.cpp)
    target_link_libraries(test_engine_mm6_outdoor_wire PUBLIC testing_unit engine_mm6_outdoor_wire)
    target_check_style(test_engine_mm6_outdoor_wire)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_outdoor_wire)
endif()
