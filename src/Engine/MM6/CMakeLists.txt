cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_library(engine_mm6_save_container STATIC MM6SaveContainer.cpp MM6SaveContainer.h)
target_check_style(engine_mm6_save_container)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_save_container OBJECT Tests/MM6SaveContainer_ut.cpp)
    target_link_libraries(test_engine_mm6_save_container PUBLIC testing_unit engine_mm6_save_container)
    target_check_style(test_engine_mm6_save_container)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_container)
endif()
