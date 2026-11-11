cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

add_library(engine_mm6_save_container STATIC MM6SaveContainer.cpp MM6SaveContainer.h
    MM6CoreSaveFields.cpp MM6CoreSaveFields.h MM6DurableSaveFields.cpp MM6DurableSaveFields.h
    MM6MapSaveFields.cpp MM6MapSaveFields.h MM6SaveMetadata.cpp MM6SaveMetadata.h
    MM6IndoorFaceOverrides.cpp MM6IndoorFaceOverrides.h
    MM6NpcSaveFields.cpp MM6NpcSaveFields.h
    MM6ResistanceSaveFields.cpp MM6ResistanceSaveFields.h
    MM6HeldItemSaveFields.cpp MM6HeldItemSaveFields.h
    MM6RawHeldItemSaveFields.cpp MM6RawHeldItemSaveFields.h MM6RawHeldCursorOwner.h
    MM6BuffSaveFields.cpp MM6BuffSaveFields.h
    MM6HirelingSaveFields.cpp MM6HirelingSaveFields.h
    MM6RecoverySaveFields.cpp MM6RecoverySaveFields.h MM6Calendar.h
    MM6ChestSaveFields.cpp MM6ChestSaveFields.h
    MM6ShopGuildClockFields.cpp MM6ShopGuildClockFields.h
    MM6ShopStockFields.cpp MM6ShopStockFields.h MM6ShopTitleState.h
    MM6ShopStockPurchase.h MM6ShopBoundedPurchase.h
    MM6ShopPhysicalPurchase.h MM6ItemRecord28.h
    MM6PlainItemProjection.h MM6StandardBonusMap.h MM6StandardItemProjection.h
    MM6BoundedItemProjection.h MM6GroundItemProjection.h MM6BoundedInventoryProjection.h
    MM6BoundedPartyInventoryProjection.h
    MM6RawInventoryFields.cpp MM6RawInventoryFields.h
    MM6BeaconSaveFields.cpp MM6BeaconSaveFields.h
    MM6ShopRefreshBoundary.cpp MM6ShopRefreshBoundary.h
    MM6SaveCrossFields.h MM6KnownSaveFields.cpp MM6KnownSaveFields.h
    MM6RawSaveFields.cpp MM6RawSaveFields.h)
target_check_style(engine_mm6_save_container)

add_library(engine_mm6_atomic_save STATIC MM6AtomicSaveFile.cpp MM6AtomicSaveFile.h)
target_link_libraries(engine_mm6_atomic_save PUBLIC engine_mm6_save_container)
target_check_style(engine_mm6_atomic_save)

add_library(engine_mm6_save_runtime STATIC MM6CoreSaveRuntime.cpp MM6CoreSaveRuntime.h
    MM6BeaconSaveRuntime.cpp MM6BeaconSaveRuntime.h
    MM6DurableSaveRuntime.cpp MM6SaveMetadataRuntime.cpp MM6ResistanceSaveRuntime.cpp
    MM6HeldItemSaveRuntime.cpp MM6BuffSaveRuntime.cpp MM6HirelingSaveRuntime.cpp MM6HirelingSaveRuntime.h
    MM6RecoverySaveRuntime.cpp MM6RecoverySaveRuntime.h MM6MapSaveValidation.cpp MM6MapSaveValidation.h
    MM6NpcStaticBaseline.cpp MM6NpcStaticBaseline.h
    MM6NpcResourceIdentity.cpp MM6NpcResourceIdentity.h
    MM6PostMapGeneratedNpcLinks.cpp MM6PostMapGeneratedNpcLinks.h
    MM6GeneratedNpcSaveFields.cpp MM6GeneratedNpcSaveFields.h
    MM6PreEventGeneratedNpcReplay.cpp MM6PreEventGeneratedNpcReplay.h
    MM6PreEventActorReplay.cpp MM6PreEventActorReplay.h
    MM6V3PreEventReplayForTest.cpp MM6V3PreEventReplayForTest.h
    MM6GameplayRngSaveFields.cpp MM6GameplayRngSaveFields.h
    MM6V3SaveFields.cpp MM6V3SaveFields.h
    MM6V3OutdoorActorMapAgreement.cpp MM6V3OutdoorActorMapAgreement.h
    MM6V3FullSessionPreflight.cpp MM6V3FullSessionPreflight.h
    MM6EventRuntimeSaveFields.cpp MM6EventRuntimeSaveFields.h
    MM6EventRuntimeRestoreForTest.cpp MM6EventRuntimeRestoreForTest.h
    MM6V4SaveFields.cpp MM6V4SaveFields.h
    MM6V4FullSessionPreflight.cpp MM6V4FullSessionPreflight.h
    MM6V5FullSessionPreflight.cpp MM6V5FullSessionPreflight.h
    MM6V5PreEventActorReplayForTest.cpp MM6V5PreEventActorReplayForTest.h
    MM6V5PreEventDecorationReplayForTest.cpp MM6V5PreEventDecorationReplayForTest.h
    MM6V5PreEventSpriteReplayForTest.cpp MM6V5PreEventSpriteReplayForTest.h
    MM6V4OutdoorDecorationReplay.cpp MM6V4OutdoorDecorationReplay.h
    MM6V5SpriteRuntimeFields.cpp MM6V5SpriteRuntimeFields.h
    MM6V5ActorRuntimeFields.cpp MM6V5ActorRuntimeFields.h
    MM6V5DecorationRuntimeFields.cpp MM6V5DecorationRuntimeFields.h
    MM6V5SaveFields.cpp MM6V5SaveFields.h
    MM6V6OutdoorSaveFields.cpp MM6V6OutdoorSaveFields.h MM6VisitedOutdoorRuntimeOwner.h
    MM6VisitedOutdoorReturnPlan.h
    MM6VisitedIndoorRuntimeOwner.h
    MM6VisitedMapCache.cpp MM6VisitedMapCache.h
    MM6VisitedMapSection.cpp MM6VisitedMapSection.h
    MM6TransitionPostWorldCaptureForTest.h
    MM6V5LiveSaveCapture.cpp MM6V5LiveSaveCapture.h
    MM6V5FirstMapSaveWriter.cpp MM6V5FirstMapSaveWriter.h
    MM6V5FirstMapNativeSlotPolicy.cpp MM6V5FirstMapNativeSlotPolicy.h
    MM6FirstMapManualSave.cpp MM6FirstMapManualSave.h
    MM6NewGameSession.cpp MM6NewGameSession.h
    MM6V5FirstMapLoadPreflight.cpp MM6V5FirstMapLoadPreflight.h
    MM6V5FirstMapLoadAttempt.cpp MM6V5FirstMapLoadAttempt.h
    MM6V6IndoorSaveFields.cpp MM6V6IndoorSaveFields.h MM6IndoorFaceLiveOwner.h
    MM6V6IndoorPreEventOwnerGate.cpp MM6V6IndoorPreEventOwnerGate.h
    MM6V6IndoorDecorationRuntime.cpp MM6V6IndoorDecorationRuntime.h
    MM6V6IndoorSpriteRuntime.cpp MM6V6IndoorSpriteRuntime.h
    MM6V6IndoorActorRuntime.cpp MM6V6IndoorActorRuntime.h
    MM6V6PreEventIndoorDecorationReplayForTest.h
    MM6V6PreEventIndoorSpriteReplayForTest.h
    MM6V6PreEventIndoorActorReplayForTest.h
    MM6ActiveActorOutdoorDeltaCapture.cpp MM6ActiveActorOutdoorDeltaCapture.h
    MM6IndoorWorldPlan.cpp MM6IndoorWorldPlan.h
    MM6WorldSaveRuntime.cpp MM6WorldSaveRuntime.h
    MM6RawSaveRuntime.cpp MM6RawSaveRuntime.h
    MM6SavedActorPidPolicy.h
    MM6PostRebuildPidSlots.cpp MM6PostRebuildPidSlots.h
    MM6PostWorldAdmissionForTest.h
    MM6PostWorldOwnerValidator.cpp MM6PostWorldOwnerValidator.h
    MM6RawCursorShopStage.cpp MM6RawCursorShopStage.h
    MM6RawCursorShopProcessOwner.cpp MM6RawCursorShopProcessOwner.h
    MM6PaperdollEquip.cpp MM6PaperdollEquip.h
    MM6RawHeldPartyStage.cpp MM6RawHeldPartyStage.h
    MM6RawCursorImagePlan.cpp MM6RawCursorImagePlan.h
    MM6RawSessionStage.cpp MM6RawSessionStage.h
    MM6RawChildPublicationForTest.cpp MM6RawChildPublicationForTest.h
    MM6PersistentEventBoundary.h
    MM6PersistentEventPublication.cpp MM6PersistentEventPublication.h
    MM6RawPartyInventoryStage.cpp MM6RawPartyInventoryStage.h
    MM6NewGameSaveRuntime.cpp MM6NewGameSaveRuntime.h
    MM6RawNewGameSaveRuntime.cpp MM6RawNewGameSaveRuntime.h MM6CreationItemSeed.h
    MM6FirstMapWorldTransaction.h
    MM6FirstMapPlacementTransaction.h
    MM6FirstMapCombatTransientTransaction.h
    MM6FirstMapPersistentEventTransaction.h
    MM6OutdoorBorderTravel.h
    MM6OutdoorBorderEligibility.h
    MM6OutdoorBorderConfirmation.h
    MM6MapTransition.cpp MM6MapTransition.h
    MM6ParkedMapSection.cpp MM6ParkedMapSection.h
    MM6SessionSave.cpp MM6SessionSave.h
    MM6ScopedNewGameSave.cpp MM6ScopedNewGameSave.h
    MM6PartyDeath.cpp MM6PartyDeath.h
    MM6PartyRest.cpp MM6PartyRest.h
    MM6LoadedMapSource.cpp MM6LoadedMapSource.h
    MM6PrivateMapSource.cpp MM6PrivateMapSource.h)
target_link_libraries(engine_mm6_save_runtime PUBLIC engine_mm6_save_container engine_mm6_atomic_save
    engine engine_components_random library_json)
target_check_style(engine_mm6_save_runtime)

add_library(engine_mm6_outdoor_wire STATIC MM6OutdoorWire.cpp MM6OutdoorWire.h
    MM6NativeActorResistanceBytes.h)
target_check_style(engine_mm6_outdoor_wire)

add_library(engine_mm6_outdoor_actor_append STATIC
    MM6OutdoorActorAppendCodec.cpp MM6OutdoorActorAppendCodec.h)
target_link_libraries(engine_mm6_outdoor_actor_append PUBLIC engine_mm6_outdoor_wire engine_objects)
target_check_style(engine_mm6_outdoor_actor_append)

add_library(engine_mm6_spawn_actor_section STATIC
    MM6SpawnedActorSection.cpp MM6SpawnedActorSection.h
    MM6ActiveActorSection.cpp MM6ActiveActorSection.h)
target_link_libraries(engine_mm6_spawn_actor_section PUBLIC engine_serialization)
target_check_style(engine_mm6_spawn_actor_section)
target_link_libraries(engine_mm6_save_runtime PUBLIC engine_mm6_spawn_actor_section)

# Engine.cpp calls the guarded MM6 pre-event/runtime entry points even in
# title-locked app targets. Keep their definitions in the runtime library and
# make every app link that library; the hooks remain inert without a GameTest
# scope and for MM7/MM8 titles.
target_link_libraries(application PUBLIC engine_mm6_save_runtime)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_actual_stat_condition OBJECT Tests/MM6ActualStatCondition_ut.cpp)
    target_link_libraries(test_engine_mm6_actual_stat_condition PUBLIC testing_unit)
    target_check_style(test_engine_mm6_actual_stat_condition)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_actual_stat_condition)

    add_library(test_engine_mm6_creation_vitals OBJECT Tests/MM6CreationVitals_ut.cpp)
    target_link_libraries(test_engine_mm6_creation_vitals PUBLIC testing_unit)
    target_check_style(test_engine_mm6_creation_vitals)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_creation_vitals)

    add_library(test_engine_mm6_creation_item_seed OBJECT Tests/MM6CreationItemSeed_ut.cpp)
    target_link_libraries(test_engine_mm6_creation_item_seed PUBLIC testing_unit engine_mm6_save_runtime)
    target_check_style(test_engine_mm6_creation_item_seed)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_creation_item_seed)

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
        Tests/MM6ChestFirstOpenTransaction_ut.cpp Tests/MM6ChestOpenOwnerTransaction_ut.cpp
        Tests/MM6ChestTrapDecision_ut.cpp
        Tests/MM6ChestDisarmScore_ut.cpp Tests/MM6ChestMapLoadRoute_ut.cpp)
    target_link_libraries(test_engine_mm6_chest_record PUBLIC testing_unit engine_tables engine_objects engine_serialization)
    target_check_style(test_engine_mm6_chest_record)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_chest_record)

    add_library(test_engine_mm6_spell_items OBJECT Tests/MM6SpellItems_ut.cpp Tests/MM6SpellLearning_ut.cpp
        Tests/MM6SpellEffectGuard_ut.cpp)
    target_link_libraries(test_engine_mm6_spell_items PUBLIC testing_unit)
    target_check_style(test_engine_mm6_spell_items)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_spell_items)

    add_library(test_engine_mm6_monster_damage OBJECT Tests/MM6MonsterDamage_ut.cpp Tests/MM6CombatRules_ut.cpp
        Tests/MM6EventSpell_ut.cpp Tests/MM6MonsterAttack_ut.cpp)
    target_link_libraries(test_engine_mm6_monster_damage PUBLIC testing_unit)
    target_check_style(test_engine_mm6_monster_damage)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_monster_damage)

    add_library(test_engine_mm6_save_container OBJECT Tests/MM6SaveContainer_ut.cpp Tests/MM6CoreSaveFields_ut.cpp
        Tests/MM6DurableSaveFields_ut.cpp Tests/MM6MapSaveFields_ut.cpp Tests/MM6SaveMetadata_ut.cpp
        Tests/MM6NpcSaveFields_ut.cpp Tests/MM6ResistanceSaveFields_ut.cpp
        Tests/MM6HeldItemSaveFields_ut.cpp Tests/MM6RawHeldItemSaveFields_ut.cpp Tests/MM6RawHeldCursorOwner_ut.cpp
        Tests/MM6BuffSaveFields_ut.cpp
        Tests/MM6IndoorFaceOverrides_ut.cpp
        Tests/MM6HirelingSaveFields_ut.cpp Tests/MM6RecoverySaveFields_ut.cpp Tests/MM6Calendar_ut.cpp
        Tests/MM6ChestSaveFields_ut.cpp Tests/MM6ShopGuildClockFields_ut.cpp
        Tests/MM6ShopStockFields_ut.cpp Tests/MM6ShopBoundedPurchase_ut.cpp
        Tests/MM6ShopPhysicalPurchase_ut.cpp
        Tests/MM6ItemRecord28_ut.cpp Tests/MM6PlainItemProjection_ut.cpp
        Tests/MM6BoundedInventoryProjection_ut.cpp
        Tests/MM6BoundedPartyInventoryProjection_ut.cpp Tests/MM6GroundItemProjection_ut.cpp Tests/MM6StandardBonusMap_ut.cpp
        Tests/MM6StandardItemProjection_ut.cpp
        Tests/MM6RawInventoryFields_ut.cpp
        Tests/MM6RawSaveFields_ut.cpp
        Tests/MM6BackpackItemEventRemoval_ut.cpp
        Tests/MM6KnownSaveFields_ut.cpp
        Tests/MM6PotionCombinations_ut.cpp)
    target_link_libraries(test_engine_mm6_save_container PUBLIC testing_unit engine_mm6_save_container engine_tables)
    target_check_style(test_engine_mm6_save_container)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_container)

    add_library(test_engine_mm6_save_runtime OBJECT Tests/MM6MapSaveValidation_ut.cpp
        Tests/MM6RawSaveRuntime_ut.cpp Tests/MM6PersistentEventBoundary_ut.cpp
        Tests/MM6PersistentEventPublication_ut.cpp
        Tests/MM6RawPartyInventoryStage_ut.cpp
        Tests/MM6RawCursorShopStage_ut.cpp
        Tests/MM6RawCursorShopProcessOwner_ut.cpp
        Tests/MM6RawHeldPartyStage_ut.cpp
        Tests/MM6RawChildPublicationForTest_ut.cpp
        Tests/MM6RawCursorImagePlan_ut.cpp
        Tests/MM6SavedActorPidPolicy_ut.cpp
        Tests/MM6PostRebuildPidSlots_ut.cpp
        Tests/MM6PostWorldAdmissionForTest_ut.cpp
        Tests/MM6V5FirstMapLoadAttempt_ut.cpp
        Tests/MM6PreEventCheckpointForTest_ut.cpp
        Tests/MM6PostWorldOwnerValidator_ut.cpp
        Tests/MM6DecodedEffectPartyStage_ut.cpp
        Tests/MM6AntiqueItemProjection_ut.cpp
        Tests/MM6BeaconSaveFields_ut.cpp
        Tests/MM6PrivateMapSource_ut.cpp Tests/MM6WorldSaveRuntime_ut.cpp
        Tests/MM6NewGameSaveRuntime_ut.cpp
        Tests/MM6RawNewGameSaveRuntime_ut.cpp
        Tests/MM6NpcStaticBaseline_ut.cpp
        Tests/MM6NpcResourceIdentity_ut.cpp
        Tests/MM6PostMapGeneratedNpcLinks_ut.cpp
        Tests/MM6GeneratedNpcSaveFields_ut.cpp
        Tests/MM6PreEventGeneratedNpcReplay_ut.cpp
        Tests/MM6V3PreEventReplayForTest_ut.cpp
        Tests/MM6GameplayRngSaveFields_ut.cpp
        Tests/MM6V3SaveFields_ut.cpp
        Tests/MM6V3OutdoorActorMapAgreement_ut.cpp
        Tests/MM6V3FullSessionPreflight_ut.cpp
        Tests/MM6EventRuntimeSaveFields_ut.cpp
        Tests/MM6EventRuntimeRestoreForTest_ut.cpp
        Tests/MM6V4SaveFields_ut.cpp
        Tests/MM6V4FullSessionPreflight_ut.cpp
        Tests/MM6V5SpriteRuntimeFields_ut.cpp
        Tests/MM6V5ActorRuntimeFields_ut.cpp
        Tests/MM6V5DecorationRuntimeFields_ut.cpp
        Tests/MM6V6IndoorSaveFields_ut.cpp
        Tests/MM6V5PreEventSpriteReplayForTest_ut.cpp
        Tests/MM6V5PreEventActorReplayForTest_ut.cpp
        Tests/MM6V5PreEventDecorationReplayForTest_ut.cpp
        Tests/MM6V5LaterOutdoorReplay_ut.cpp
        Tests/MM6OutdoorBorderTravel_ut.cpp
        Tests/MM6OutdoorBorderEligibility_ut.cpp
        Tests/MM6OutdoorBorderPrecommit_ut.cpp
        Tests/MM6OutdoorBorderConfirmation_ut.cpp
        Tests/MM6OutdoorReturnArrival_ut.cpp
        Tests/MM6VisitedOwnerFixtures.h
        Tests/MM6VisitedIndoorRuntimeOwner_ut.cpp
        Tests/MM6VisitedMapCache_ut.cpp
        Tests/MM6VisitedMapSection_ut.cpp
        Tests/MM6ParkedMapSection_ut.cpp
        Tests/MM6MapEnumFunctions_ut.cpp
        Tests/MM6VisitedReturnPreEventProbeForTest_ut.cpp
        Tests/MM6TransitionAutosaveInterceptForTest_ut.cpp
        Tests/MM6V5OutdoorDecorationSource_ut.cpp
        Tests/MM6V5DerivedActorNames_ut.cpp
        Tests/MM6MapStatsEncounterColumns_ut.cpp
        Tests/MM6V4OutdoorDecorationReplay_ut.cpp
        Tests/MM6IndoorWorldPlan_ut.cpp
        Tests/MM6HirelingRecoveryV2Stage_ut.cpp
        Tests/MM6FirstMapCombatTransientTransaction_ut.cpp
        Tests/MM6FirstMapPersistentEventTransaction_ut.cpp)
    target_sources(test_engine_mm6_save_runtime PRIVATE Tests/MM6PartyDeath_ut.cpp Tests/MM6Archive_ut.cpp)
    target_link_libraries(test_engine_mm6_save_runtime PUBLIC testing_unit engine_mm6_save_runtime)
    target_check_style(test_engine_mm6_save_runtime)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_save_runtime)

    add_library(test_engine_mm6_atomic_save OBJECT Tests/MM6AtomicSaveFile_ut.cpp)
    target_link_libraries(test_engine_mm6_atomic_save PUBLIC testing_unit engine_mm6_atomic_save
        engine_mm6_save_runtime)
    target_check_style(test_engine_mm6_atomic_save)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_atomic_save)

    add_library(test_engine_mm6_outdoor_wire OBJECT Tests/MM6OutdoorWire_ut.cpp
        Tests/MM6NativeActorResistanceBytes_ut.cpp)
    target_link_libraries(test_engine_mm6_outdoor_wire PUBLIC testing_unit engine_mm6_outdoor_wire)
    target_check_style(test_engine_mm6_outdoor_wire)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_outdoor_wire)

    add_library(test_engine_mm6_outdoor_actor_append OBJECT Tests/MM6OutdoorActorAppendCodec_ut.cpp)
    target_link_libraries(test_engine_mm6_outdoor_actor_append PUBLIC testing_unit engine_mm6_outdoor_actor_append)
    target_check_style(test_engine_mm6_outdoor_actor_append)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_outdoor_actor_append)

    add_library(test_engine_mm6_spawn_actor_section OBJECT Tests/MM6SpawnedActorSection_ut.cpp
        Tests/MM6ActiveActorSection_ut.cpp)
    target_link_libraries(test_engine_mm6_spawn_actor_section PUBLIC testing_unit engine_mm6_spawn_actor_section)
    target_check_style(test_engine_mm6_spawn_actor_section)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_spawn_actor_section)
endif()

add_library(engine_mm6_item_generator STATIC MM6ItemGenerator.cpp MM6ItemGenerator.h)
target_link_libraries(engine_mm6_item_generator PUBLIC engine_objects engine_tables)
target_check_style(engine_mm6_item_generator)

if(OE_BUILD_TESTS)
    add_library(test_engine_mm6_item_generator OBJECT Tests/MM6ItemGenerator_ut.cpp)
    target_link_libraries(test_engine_mm6_item_generator PUBLIC testing_unit engine_mm6_item_generator)
    target_check_style(test_engine_mm6_item_generator)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_engine_mm6_item_generator)
endif()
