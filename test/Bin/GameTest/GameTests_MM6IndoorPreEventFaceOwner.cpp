#include <algorithm>
#include <cstdlib>
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
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6IndoorFaceLiveOwner.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/GUIWindow.h"

namespace {

struct Source {
    std::string name;
    IndoorLocation_MM7 base;
};

Source indoorSource() {
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto names = maps.expectedNames();
    const auto entry = std::find_if(names.begin(), names.end(), [](const std::string &name) {
        return name.ends_with(".dlv");
    });
    if (entry == names.end())
        throw std::runtime_error("MM6 pre-event face probe has no indoor map pair");
    std::string name = entry->substr(0, entry->size() - 4) + ".blv";
    const auto parsed = deserializeMM6IndoorBase(maps.readBase(name));
    (void) decodeMM6IndoorRuntimeDelta(
        decodeMM6ArchiveEntry(maps.readDefaultDelta(*entry)), parsed);
    return {std::move(name), projectMM6IndoorBase(parsed)};
}

} // namespace

// Run alone in a fresh null-renderer process. The real DoPrepareWorld loading
// route calls this checkpoint after BLV/DLV reconstruction and clickable-face
// repair, but before OnMapReload, timer unpause, or input. No save is loaded.
GAME_TEST(MM6IndoorPreEventFaceOwner, DISABLED_CapturesRebuiltFaceBaselineBeforeEvents) {
    const char *enable = std::getenv("ENROTH_MM6_INDOOR_PRE_EVENT_ENABLE");
    if (!enable || std::string_view(enable) != "strict-indoor-face-baseline")
        GTEST_SKIP() << "Opt in to the disposable indoor pre-event probe";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    const Source source = indoorSource();
    const MapId map = pMapTable->GetMapInfo(source.name);
    ASSERT_NE(map, MAP_INVALID);

    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    ASSERT_TRUE(pMapDeltas.contains(source.name.substr(0, source.name.size() - 4) + ".dlv"));

    bool completed = false;
    bool captured = false;
    bool aborted = false;
    std::vector<mm6::IndoorFaceState> baseline;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6,
            [&] {
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 indoor face checkpoint preceded BLV reconstruction");
                baseline = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, source.base, GameTitle::MM6);
                if (mm6::captureIndoorFaceOverrides(
                        source.name, baseline, baseline, GameTitle::MM6))
                    throw std::runtime_error("MM6 indoor face baseline differs from itself");
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
    ASSERT_TRUE(aborted);
    ASSERT_TRUE(captured);
    ASSERT_FALSE(baseline.empty());
    ASSERT_TRUE(checkpoint->reachedMapLoad() == false);
    ASSERT_TRUE(checkpoint->reachedInput() == false);
    ASSERT_EQ(baseline.size(), source.base.faces.size());
}
