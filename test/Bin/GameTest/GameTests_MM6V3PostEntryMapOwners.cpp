#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

// Diagnostic only. A v3 writer must not use the retained DDM as MAPS/CHST
// unless its represented live map owners still agree after world preparation.
GAME_TEST(MM6V3PostEntryMapOwnersHeadless, DISABLED_FirstMapRetainedDDMMatchesLiveOwners) {
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
    std::string resultClass = "world-preparation";
    game.runGameRoutine([&] {
        try {
            engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
            {
                mm6::ScopedChestMapLoadOptIn chestOptIn;
                DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
            }
            const MM6OutdoorDeltaSnapshot retained = decodeMM6OutdoorDelta(Blob::view(
                pOutdoor->mm6DeltaBytes.data(), pOutdoor->mm6DeltaBytes.size()));
            if (retained.lastRespawnDay == 0 ||
                retained.lastRespawnDay != static_cast<std::uint32_t>(pOutdoor->ddm.lastRespawnDay)) {
                resultClass = "respawn-day";
            } else if (retained.fullyRevealedCells != pOutdoor->uFullyRevealedCellOnMap ||
                       retained.partiallyRevealedCells != pOutdoor->uPartiallyRevealedCellOnMap) {
                resultClass = "visibility";
            } else if (retained.spriteObjects.size() != pSpriteObjects.size()) {
                resultClass = "sprite-count";
            } else if (retained.chests.size() != vChests.size()) {
                resultClass = "chest-count";
            } else {
                resultClass = "pass";
                for (size_t slot = 0; slot < pSpriteObjects.size(); ++slot) {
                    SpriteObject_MM7 live{};
                    snapshot(pSpriteObjects[slot], &live);
                    const auto *lhs = reinterpret_cast<const std::uint8_t *>(&live);
                    const auto *rhs = reinterpret_cast<const std::uint8_t *>(&retained.spriteObjects[slot]);
                    if (std::memcmp(lhs, rhs, 24) || std::memcmp(lhs + 26, rhs + 26, 10)) {
                        resultClass = "sprite-header";
                        break;
                    }
                    if (std::memcmp(lhs + 36, rhs + 36, 26)) {
                        resultClass = "sprite-item";
                        break;
                    }
                }
                if (resultClass == "pass") {
                    const auto wire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
                    mm6::ChestRecordSet records{};
                    if (!mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records)) {
                        resultClass = "chest-wire";
                    } else {
                        for (size_t slot = 0; slot < vChests.size(); ++slot) {
                            const auto replay = mm6::stageChestRuntimeRecord(GameTitle::MM6,
                                mm6::ChestRuntimeProjection{vChests[slot], records[slot]});
                            if (!replay || *replay != records[slot]) {
                                resultClass = "chest-state";
                                break;
                            }
                        }
                    }
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
    EXPECT_EQ(resultClass, "pass");
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
