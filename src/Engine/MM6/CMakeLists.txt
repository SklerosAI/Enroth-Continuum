cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_library(engine_mm6_save_container STATIC MM6SaveContainer.cpp MM6SaveContainer.h
    MM6CoreSaveFields.cpp MM6CoreSaveFields.h MM6DurableSaveFields.cpp MM6DurableSaveFields.h
    MM6MapSaveFields.cpp MM6MapSaveFields.h MM6SaveMetadata.cpp MM6SaveMetadata.h
    MM6NpcSaveFields.cpp MM6NpcSaveFields.h
    MM6ResistanceSaveFields.cpp MM6ResistanceSaveFields.h
    MM6HeldItemSaveFields.cpp MM6HeldItemSaveFields.h
    MM6BuffSaveFields.cpp MM6BuffSaveFields.h
    MM6HirelingSaveFields.cpp MM6HirelingSaveFields.h
    MM6RecoverySaveFields.cpp MM6RecoverySaveFields.h
    MM6KnownSaveFields.cpp MM6KnownSaveFields.h)
target_check_style(engine_mm6_save_container)

add_library(engine_mm6_atomic_save STATIC MM6AtomicSaveFile.cpp MM6AtomicSaveFile.h)
target_link_libraries(engine_mm6_atomic_save PUBLIC engine_mm6_save_container)
target_check_style(engine_mm6_atomic_save)

add_library(engine_mm6_save_runtime STATIC MM6CoreSaveRuntime.cpp MM6CoreSaveRuntime.h
    MM6DurableSaveRuntime.cpp MM6SaveMetadataRuntime.cpp MM6ResistanceSaveRuntime.cpp
    MM6HeldItemSaveRuntime.cpp MM6BuffSaveRuntime.cpp MM6HirelingSaveRuntime.cpp
    MM6RecoverySaveRuntime.cpp MM6MapSaveValidation.cpp MM6MapSaveValidation.h
    MM6PrivateMapSource.cpp MM6PrivateMapSource.h)
target_link_libraries(engine_mm6_save_runtime PUBLIC engine_mm6_save_container engine library_json)
target_check_style(engine_mm6_save_runtime)

add_library(engine_mm6_outdoor_wire STATIC MM6OutdoorWire.cpp MM6OutdoorWire.h)
target_check_style(engine_mm6_outdoor_wire)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_spell_items OBJECT Tests/MM6SpellItems_ut.cpp Tests/MM6SpellLearning_ut.cpp
        Tests/MM6SpellEffectGuard_ut.cpp)
    target_link_libraries(test_engine_mm6_spell_items PUBLIC testing_unit)
    target_check_style(test_engine_mm6_spell_items)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_spell_items)

    add_library(test_engine_mm6_save_container OBJECT Tests/MM6SaveContainer_ut.cpp Tests/MM6CoreSaveFields_ut.cpp
        Tests/MM6DurableSaveFields_ut.cpp Tests/MM6MapSaveFields_ut.cpp Tests/MM6SaveMetadata_ut.cpp
        Tests/MM6NpcSaveFields_ut.cpp Tests/MM6ResistanceSaveFields_ut.cpp
        Tests/MM6HeldItemSaveFields_ut.cpp Tests/MM6BuffSaveFields_ut.cpp
        Tests/MM6HirelingSaveFields_ut.cpp Tests/MM6RecoverySaveFields_ut.cpp
        Tests/MM6KnownSaveFields_ut.cpp Tests/MM6PotionCombinations_ut.cpp)
    target_link_libraries(test_engine_mm6_save_container PUBLIC testing_unit engine_mm6_save_container engine_tables)
    target_check_style(test_engine_mm6_save_container)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_container)

    add_library(test_engine_mm6_save_runtime OBJECT Tests/MM6MapSaveValidation_ut.cpp
        Tests/MM6PrivateMapSource_ut.cpp)
    target_link_libraries(test_engine_mm6_save_runtime PUBLIC testing_unit engine_mm6_save_runtime)
    target_check_style(test_engine_mm6_save_runtime)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_runtime)

    add_library(test_engine_mm6_atomic_save OBJECT Tests/MM6AtomicSaveFile_ut.cpp)
    target_link_libraries(test_engine_mm6_atomic_save PUBLIC testing_unit engine_mm6_atomic_save)
    target_check_style(test_engine_mm6_atomic_save)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_atomic_save)

    add_library(test_engine_mm6_outdoor_wire OBJECT Tests/MM6OutdoorWire_ut.cpp)
    target_link_libraries(test_engine_mm6_outdoor_wire PUBLIC testing_unit engine_mm6_outdoor_wire)
    target_check_style(test_engine_mm6_outdoor_wire)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_outdoor_wire)
endif()
