#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6SpawnedActorSection.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

namespace {

std::string_view refusalClass(std::string_view message) {
    if (message.find("item or buff") != std::string_view::npos ||
        message.find("unsupported item") != std::string_view::npos ||
        message.find("unsupported buff") != std::string_view::npos)
        return "unsupported-item-buff";
    if (message.find("narrows") != std::string_view::npos)
        return "narrowing";
    if (message.find("replay") != std::string_view::npos)
        return "replay";
    if (message.find("slot or monster") != std::string_view::npos)
        return "identity";
    return "other-refusal";
}

} // namespace

// Run alone in a fresh headless process. This reads private game data through
// the existing loader, writes no profile, and never publishes SPAW to a save.
GAME_TEST(MM6SpawnedActorSectionHeadless, DISABLED_FirstMapDetachedCapture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    bool completed = false;
    bool captured = false;
    std::string resultClass = "world-preparation";
    game.runGameRoutine([&] {
        try {
            engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
            {
                mm6::ScopedChestMapLoadOptIn chestOptIn;
                DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
            }
            const auto sidecar = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
            const std::vector<Actor> live(pActors.begin(), pActors.end());
            if (live.size() < sidecar.actorCount) {
                resultClass = "missing-baseline-slots";
            } else {
                try {
                    const auto appended = std::span(live).subspan(sidecar.actorCount);
                    const auto bytes = mm6::encodeSpawnedActorSection(
                        appended, sidecar.actorCount, GameTitle::MM6);
                    const auto replay = mm6::decodeSpawnedActorSection(
                        bytes, sidecar.actorCount, GameTitle::MM6);
                    captured = mm6::encodeSpawnedActorSection(
                        replay, sidecar.actorCount, GameTitle::MM6) == bytes;
                    resultClass = captured ? "pass" : "replay-mismatch";
                } catch (const std::exception &error) {
                    resultClass = refusalClass(error.what());
                }
            }
        } catch (const std::exception &) {
            resultClass = "world-preparation";
        }
        completed = true;
    });
    for (int tick = 0; tick < 256 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    EXPECT_TRUE(captured) << resultClass;
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
