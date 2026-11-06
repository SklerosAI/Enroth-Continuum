cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(ENGINE_SPELLS_SOURCES
        CastSpellInfo.cpp
        MM6SpellRules.cpp
        MM6FirstAidRules.cpp
        Spells.cpp)

set(ENGINE_SPELLS_HEADERS
        CastSpellInfo.h
        MM6SpellRules.h
        MM6FirstAidRules.h
        SpellBuff.h
        SpellEnums.h
        SpellEnumFunctions.h
        Spells.h)

add_library(engine_spells STATIC ${ENGINE_SPELLS_SOURCES} ${ENGINE_SPELLS_HEADERS})
target_link_libraries(engine_spells PUBLIC engine)
target_check_style(engine_spells)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_spell_rules OBJECT Tests/MM6SpellRules_ut.cpp Tests/MM6FirstAidRules_ut.cpp)
    target_link_libraries(test_engine_mm6_spell_rules PUBLIC testing_unit engine_spells)
    target_check_style(test_engine_mm6_spell_rules)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_spell_rules)

    set(TEST_ENGINE_SPELLS_SOURCES Tests/Spells_ut.cpp)
    add_library(test_engine_spells OBJECT ${TEST_ENGINE_SPELLS_SOURCES})
    target_link_libraries(test_engine_spells PUBLIC testing_unit engine_spells)
    target_check_style(test_engine_spells)
    target_link_libraries(OpenEnroth_GameTest PUBLIC test_engine_spells)
endif()
