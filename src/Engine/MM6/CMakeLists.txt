cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_library(engine_mm6_save_container STATIC MM6SaveContainer.cpp MM6SaveContainer.h
    MM6CoreSaveFields.cpp MM6CoreSaveFields.h MM6DurableSaveFields.cpp MM6DurableSaveFields.h
    MM6MapSaveFields.cpp MM6MapSaveFields.h MM6SaveMetadata.cpp MM6SaveMetadata.h
    MM6IndoorFaceOverrides.cpp MM6IndoorFaceOverrides.h
    MM6NpcSaveFields.cpp MM6NpcSaveFields.h
    MM6ResistanceSaveFields.cpp MM6ResistanceSaveFields.h
    MM6HeldItemSaveFields.cpp MM6HeldItemSaveFields.h
    MM6BuffSaveFields.cpp MM6BuffSaveFields.h
    MM6HirelingSaveFields.cpp MM6HirelingSaveFields.h
    MM6RecoverySaveFields.cpp MM6RecoverySaveFields.h
    MM6ChestSaveFields.cpp MM6ChestSaveFields.h
    MM6ShopGuildClockFields.cpp MM6ShopGuildClockFields.h
    MM6ShopStockFields.cpp MM6ShopStockFields.h MM6ShopStockPurchase.h MM6ItemRecord28.h
    MM6RawInventoryFields.cpp MM6RawInventoryFields.h
    MM6BeaconSaveFields.cpp MM6BeaconSaveFields.h
    MM6ShopRefreshBoundary.cpp MM6ShopRefreshBoundary.h
    MM6KnownSaveFields.cpp MM6KnownSaveFields.h)
target_check_style(engine_mm6_save_container)

add_library(engine_mm6_atomic_save STATIC MM6AtomicSaveFile.cpp MM6AtomicSaveFile.h)
target_link_libraries(engine_mm6_atomic_save PUBLIC engine_mm6_save_container)
target_check_style(engine_mm6_atomic_save)

add_library(engine_mm6_save_runtime STATIC MM6CoreSaveRuntime.cpp MM6CoreSaveRuntime.h
    MM6BeaconSaveRuntime.cpp MM6BeaconSaveRuntime.h
    MM6DurableSaveRuntime.cpp MM6SaveMetadataRuntime.cpp MM6ResistanceSaveRuntime.cpp
    MM6HeldItemSaveRuntime.cpp MM6BuffSaveRuntime.cpp MM6HirelingSaveRuntime.cpp
    MM6RecoverySaveRuntime.cpp MM6MapSaveValidation.cpp MM6MapSaveValidation.h
    MM6WorldSaveRuntime.cpp MM6WorldSaveRuntime.h
    MM6NewGameSaveRuntime.cpp MM6NewGameSaveRuntime.h
    MM6FirstMapWorldTransaction.h
    MM6FirstMapPlacementTransaction.h
    MM6ScopedNewGameSave.cpp MM6ScopedNewGameSave.h
    MM6PrivateMapSource.cpp MM6PrivateMapSource.h)
target_link_libraries(engine_mm6_save_runtime PUBLIC engine_mm6_save_container engine_mm6_atomic_save engine library_json)
target_check_style(engine_mm6_save_runtime)

add_library(engine_mm6_outdoor_wire STATIC MM6OutdoorWire.cpp MM6OutdoorWire.h)
target_check_style(engine_mm6_outdoor_wire)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_actual_stat_condition OBJECT Tests/MM6ActualStatCondition_ut.cpp)
    target_link_libraries(test_engine_mm6_actual_stat_condition PUBLIC testing_unit)
    target_check_style(test_engine_mm6_actual_stat_condition)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_actual_stat_condition)

    add_library(test_engine_mm6_creation_vitals OBJECT Tests/MM6CreationVitals_ut.cpp)
    target_link_libraries(test_engine_mm6_creation_vitals PUBLIC testing_unit)
    target_check_style(test_engine_mm6_creation_vitals)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_creation_vitals)

    add_library(test_engine_mm6_voice_sound OBJECT Tests/MM6VoiceSound_ut.cpp)
    target_link_libraries(test_engine_mm6_voice_sound PUBLIC testing_unit)
    target_check_style(test_engine_mm6_voice_sound)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_voice_sound)

    add_library(test_engine_mm6_chest_guard OBJECT Tests/MM6ChestGenerationGuard_ut.cpp
        Tests/MM6ChestOuterPlan_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_guard PUBLIC testing_unit engine_objects)
    target_check_style(test_engine_mm6_chest_guard)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_guard)

    add_library(test_engine_mm6_chest_caller OBJECT Tests/MM6ChestCallerAdapter_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_caller PUBLIC testing_unit engine_tables)
    target_check_style(test_engine_mm6_chest_caller)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_caller)

    add_library(test_engine_mm6_chest_grid OBJECT Tests/MM6ChestGridWire_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_grid PUBLIC testing_unit engine_mm6_save_container)
    target_check_style(test_engine_mm6_chest_grid)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_grid)

    add_library(test_engine_mm6_chest_type OBJECT Tests/MM6ChestTypeLayout_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_type PUBLIC testing_unit)
    target_check_style(test_engine_mm6_chest_type)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_type)

    add_library(test_engine_mm6_chest_record OBJECT Tests/MM6ChestRecordTransaction_ut.cpp
        Tests/MM6ChestRuntimeProjection_ut.cpp Tests/MM6ChestMapLoadHandoff_ut.cpp
        Tests/MM6ChestLiveOwnerAdapter_ut.cpp Tests/MM6ChestSeedAndOwnership_ut.cpp
        Tests/MM6ChestFirstOpenTransaction_ut.cpp Tests/MM6ChestTrapDecision_ut.cpp
        Tests/MM6ChestDisarmScore_ut.cpp Tests/MM6ChestMapLoadRoute_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_record PUBLIC testing_unit engine_tables engine_objects engine_serialization)
    target_check_style(test_engine_mm6_chest_record)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_record)

    add_library(test_engine_mm6_spell_items OBJECT Tests/MM6SpellItems_ut.cpp Tests/MM6SpellLearning_ut.cpp
        Tests/MM6SpellEffectGuard_ut.cpp)
    target_link_libraries(test_engine_mm6_spell_items PUBLIC testing_unit)
    target_check_style(test_engine_mm6_spell_items)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_spell_items)

    add_library(test_engine_mm6_monster_damage OBJECT Tests/MM6MonsterDamage_ut.cpp)
    target_link_libraries(test_engine_mm6_monster_damage PUBLIC testing_unit)
    target_check_style(test_engine_mm6_monster_damage)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_monster_damage)

    add_library(test_engine_mm6_save_container OBJECT Tests/MM6SaveContainer_ut.cpp Tests/MM6CoreSaveFields_ut.cpp
        Tests/MM6DurableSaveFields_ut.cpp Tests/MM6MapSaveFields_ut.cpp Tests/MM6SaveMetadata_ut.cpp
        Tests/MM6NpcSaveFields_ut.cpp Tests/MM6ResistanceSaveFields_ut.cpp
        Tests/MM6HeldItemSaveFields_ut.cpp Tests/MM6BuffSaveFields_ut.cpp
        Tests/MM6IndoorFaceOverrides_ut.cpp
        Tests/MM6HirelingSaveFields_ut.cpp Tests/MM6RecoverySaveFields_ut.cpp
        Tests/MM6ChestSaveFields_ut.cpp Tests/MM6ShopGuildClockFields_ut.cpp
        Tests/MM6ShopStockFields_ut.cpp
        Tests/MM6ItemRecord28_ut.cpp
        Tests/MM6RawInventoryFields_ut.cpp
        Tests/MM6KnownSaveFields_ut.cpp
        Tests/MM6PotionCombinations_ut.cpp)
    target_link_libraries(test_engine_mm6_save_container PUBLIC testing_unit engine_mm6_save_container engine_tables)
    target_check_style(test_engine_mm6_save_container)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_container)

    add_library(test_engine_mm6_save_runtime OBJECT Tests/MM6MapSaveValidation_ut.cpp
        Tests/MM6BeaconSaveFields_ut.cpp
        Tests/MM6PrivateMapSource_ut.cpp Tests/MM6WorldSaveRuntime_ut.cpp
        Tests/MM6NewGameSaveRuntime_ut.cpp)
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
