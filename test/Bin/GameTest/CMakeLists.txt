cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(GAME_TEST_MAIN_SOURCES
        GameTestCommon.cpp
        GameTestMain.cpp
        GameTestOptions.cpp
        MM6HeadlessRules.cpp
        MM7CampaignChain.cpp
        GameTests_0000.cpp
        GameTests_0500.cpp
        GameTests_1000.cpp
        GameTests_1500.cpp
        GameTests_2000.cpp
        GameTests_2500.cpp
        GameTests_MM7Vendor.cpp
        GameTests_MM7Combat.cpp
        GameTests_MM7Campaign.cpp
        GameTests_MM7CastlePath.cpp
        GameTests_MM7CastleRebuild.cpp
        GameTests_MM7ScavengerPath.cpp
        GameTests_MM7CampaignChainC0.cpp
        GameTests_MM7CampaignChainC1.cpp
        GameTests_MM7CampaignChainC2.cpp
        GameTests_MM7LightWalls.cpp
        GameTests_MM7LightAdvisors.cpp
        GameTests_MM7LightXenofex.cpp
        GameTests_MM7LightSeam.cpp
        GameTests_MM7LightAssassination.cpp
        GameTests_MM7OverthrusterRoute.cpp
        GameTests_MM7PitCubeAcquisition.cpp
        GameTests_MM7XenofexCombat.cpp
        GameTests_MM7LightItems.cpp
        GameTests_MM7Scrolls.cpp
        GameTests_MM6.cpp
        GameTests_MM6CharacterBitEvent.cpp
        GameTests_MM6EventVariables.cpp
        GameTests_MM6Creation.cpp
        GameTests_MM6FlyingEvent.cpp
        GameTests_MM6IndoorFacetEvent.cpp
        GameTests_MM6MonthEvent.cpp
        GameTests_MM6SkillEvent.cpp
        GameTests_MM6OriginalNoopEvent.cpp
        GameTests_MM6Save.cpp
        GameTests_MM6Houses.cpp)
set(GAME_TEST_MAIN_HEADERS
        GameTestCommon.h
        GameTestOptions.h
        MM6HeadlessRules.h
        MM7CampaignChain.h)

add_executable(OpenEnroth_GameTest ${GAME_TEST_MAIN_SOURCES} ${GAME_TEST_MAIN_HEADERS})
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6Chest.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6SaveDispatch.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6MainMenuQuickLoadGuard.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6CreationItemSeed.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6PostWorldAbort.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6PreEventCheckpoint.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V3PreEventReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6RawV2FirstMap.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6SpawnedActorSection.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V3PostEntryMapOwners.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V3PostEntryCandidate.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V4PostEntryCandidate.cpp)
target_sources(OpenEnroth_GameTest PRIVATE MM6V4AfterActionCaptureForTest.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V4FreshChildReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V4AfterActionFreshChildReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5AfterActionFreshChildReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5SecondFreshChildReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6IndoorFaceFreshChild.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5OutdoorSourceAudit.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5LaterOutdoorLoadProbe.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6WorldLoadProbe.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5NormalLoadMenu.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6IndoorPreEventFaceOwner.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V6IndoorStage.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5LaterOutdoorCandidate.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V6IndoorFreshChild.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6V5LaterOutdoorFreshChildReplay.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6LocationChangeLifecycle.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6NpcResourceIdentity.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6HudAudit.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6TopPanel.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6PortraitAlert.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6PortraitSelection.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6DrawGUI.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6OffscreenFrame.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6Transition.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM7OffscreenFrame.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM7DisposableSave.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6Combat.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6GiveItemEvent.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6BorderRest.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6Calendar.cpp)
target_sources(OpenEnroth_GameTest PRIVATE GameTests_MM6CharacterScreen.cpp)
target_link_libraries(OpenEnroth_GameTest PUBLIC application testing_game library_cli library_platform_main library_stack_trace
        engine_mm6_save_runtime engine_mm6_spawn_actor_section)

target_check_style(OpenEnroth_GameTest)

add_custom_target(Run_GameTest
        OpenEnroth_GameTest --test-path ${OE_TESTDATA_PATH}
        DEPENDS OpenEnroth_GameTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL) # USES_TERMINAL makes the command print progress as it goes.

add_custom_target(Run_GameTest_Headless
        OpenEnroth_GameTest --test-path ${OE_TESTDATA_PATH} --headless
        DEPENDS OpenEnroth_GameTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL)

add_custom_target(Run_GameTest_Parallel
        Python::Interpreter ${CMAKE_SOURCE_DIR}/thirdparty/gtest_parallel/gtest-parallel --print_test_times
            $<TARGET_FILE:OpenEnroth_GameTest> -- --test-path ${OE_TESTDATA_PATH}
        DEPENDS OpenEnroth_GameTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL)

add_custom_target(Run_GameTest_Headless_Parallel
        Python::Interpreter ${CMAKE_SOURCE_DIR}/thirdparty/gtest_parallel/gtest-parallel --print_test_times
            $<TARGET_FILE:OpenEnroth_GameTest> -- --test-path ${OE_TESTDATA_PATH} --headless
        DEPENDS OpenEnroth_GameTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL)
