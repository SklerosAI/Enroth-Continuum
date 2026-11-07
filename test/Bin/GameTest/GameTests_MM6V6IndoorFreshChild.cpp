#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6EventRuntimeSaveFields.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6IndoorFaceLiveOwner.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "GUI/GUIWindow.h"

#include "MM6V6IndoorProbeSlot.h"

// Run only after the strict v6 disposable writer, in a distinct child. It
// reconstructs the saved DLV through the real indoor loader and observes its
// pre-OnMapReload face base, then re-stages every detached v6 owner. It never
// applies the candidate to live Party/actors, admits input, or calls SaveLoad.
GAME_TEST(MM6V6IndoorFreshChild, DISABLED_RestagesSeparateCandidateBeforeEvents) {
    const char *enable = std::getenv("ENROTH_MM6_V6_INDOOR_READER_ENABLE");
    if (!enable || std::string_view(enable) != "strict-indoor-v6-reader")
        GTEST_SKIP() << "Run after strict disposable v6 indoor writer";
    const auto profile = mm6::testing::indoorProbeProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = mm6::testing::readIndoorProbeSlot(
        profile, mm6::testing::V6_INDOOR_PROBE_SLOT);
    const auto seedBefore = mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6");
    auto decoded = std::unique_ptr<mm6::V6IndoorSaveFields>(
        new mm6::V6IndoorSaveFields(mm6::decodeV6IndoorSave(before, GameTitle::MM6)));
    const std::string mapName = decoded->raw.metadata.header.mapFileName;
    ASSERT_TRUE(mapName.ends_with(".blv"));
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const MapId map = pMapTable->GetMapInfo(mapName);
    ASSERT_NE(map, MAP_INVALID);
    const auto projectedBase = projectMM6IndoorBase(
        deserializeMM6IndoorBase(maps.readBase(mapName)));

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const auto &entry : decoded->raw.maps.deltas)
        pMapDeltas.emplace(entry.name, Blob::copy(entry.bytes.data(), entry.bytes.size()));

    bool completed = false;
    bool captured = false;
    bool aborted = false;
    std::vector<mm6::IndoorFaceState> preEventFaces;
    std::vector<std::uint8_t> rebuiltDelta;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6,
            [&] {
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 reader checkpoint preceded indoor rebuild");
                preEventFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
                rebuiltDelta = pIndoor->mm6DeltaBytes;
                captured = true;
            },
            mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        try {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(true, 1);
        } catch (const mm6::testing::PreEventCheckpointAbortForTest &) {
            aborted = true;
        }
        completed = true;
    });
    for (int tick = 0; tick < 128 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(captured);
    ASSERT_TRUE(aborted);
    ASSERT_FALSE(checkpoint->reachedMapLoad());
    ASSERT_FALSE(checkpoint->reachedInput());
    ASSERT_EQ(rebuiltDelta, decoded->raw.chest.rawActiveDelta);

    bool stageFinished = false;
    std::exception_ptr stageError;
    std::unique_ptr<mm6::StagedV6IndoorSaveFields> staged;
    game.runGameRoutine([&] {
        try {
            const auto freshNpcs = mm6::loadNpcStaticBaseline(
                *engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            const auto readFaces = [&mapName, &preEventFaces](std::string_view requested) {
                if (requested != mapName)
                    throw std::runtime_error("MM6 v6 reader requested another face source");
                return preEventFaces;
            };
            staged.reset(new mm6::StagedV6IndoorSaveFields(mm6::stageV6IndoorSave(
                before, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), readFaces, GameTitle::MM6)));
            (void) mm6::requireEventRuntimeMapEntry(
                staged->saved.eventRuntime, mapName,
                pIndoor->dlv.respawnCount, pIndoor->dlv.lastRespawnDay,
                pLevelDecorations, GameTitle::MM6);
            mm6::requireGeneratedNpcActorAgreement(
                staged->saved.generatedNpcs, pActors, mapName, GameTitle::MM6);
        } catch (...) {
            stageError = std::current_exception();
        }
        stageFinished = true;
    });
    for (int tick = 0; tick < 128 && !stageFinished; ++tick)
        game.tick();
    ASSERT_TRUE(stageFinished);
    if (stageError)
        std::rethrow_exception(stageError);
    ASSERT_TRUE(staged);
    ASSERT_TRUE(staged->represented.indoorPlan);
    EXPECT_FALSE(staged->represented.outdoorPlan);
    EXPECT_FALSE(staged->canEnterGameLoop());
    EXPECT_EQ(mm6::encodeV6IndoorSave(staged->saved, GameTitle::MM6), before);
    auto afterEvent = preEventFaces;
    mm6::applyIndoorFaceOverridesAfterMapReload(
        staged->saved.faceOverrides, mapName, &afterEvent, GameTitle::MM6);
    EXPECT_EQ(afterEvent.front().attributes, preEventFaces.front().attributes ^ 1u);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(
        profile, mm6::testing::V6_INDOOR_PROBE_SLOT), before);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), seedBefore);
}
