#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/MapTable.h"

#include "Utility/Memory/Blob.h"

// This is intentionally separate from GameTests_MM6.cpp, which owns the
// broader first-map preflight. Run only with the MM6 headless GameTest profile.
GAME_TEST(MM6ZChestHeadless, ScopedFirstOutdoorMapAndRollback) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(mm6::chestMapLoadOptedIn());

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);

    // The headless profile has already mounted its private Games archive.
    // Seed only decoded archive defaults into the test's in-memory map owner.
    // No game installation or save path is written by this test.
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_EQ(pMapDeltas.size(), 67u);
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));

    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    // Exercise actual first-map archive/delta reconstruction and the live
    // chest route. The outer outdoor load also spawns encounters; that path
    // has an independent MM6 monster-table blocker.
    bool committed = false;
    game.runGameRoutine([firstMap, &committed] {
        pParty->Reset();
        engine->_currentLoadedMapId = firstMap;
        uCurrentlyLoadedLevelType = LEVEL_OUTDOOR;
        bool respawned = false;
        pOutdoor->Load("oute3.odm", 1, 0, &respawned);
        mm6::ScopedChestMapLoadOptIn optIn;
        committed = mm6::tryActiveChestMapLoad(GameTitle::MM6, firstMap, pMapTable, pItemTable, pParty);
    });
    ASSERT_TRUE(committed);
    ASSERT_FALSE(mm6::chestMapLoadOptedIn());
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    ASSERT_TRUE(mm6::chestTitleState().ready);
    ASSERT_EQ(vChests.size(), 20u);

    const auto wire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
    ASSERT_EQ(wire.chestCount, 20u);
    mm6::ChestRecordSet records{};
    ASSERT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    for (int index = 0; index < 20; ++index) {
        Chest_MM7 live{};
        snapshot(vChests[index], &live);
        EXPECT_EQ(live.chestTypeId, mm6::readChest16(records[index], 0));
        EXPECT_EQ(live.flags, mm6::readChest16(records[index], 2));
        for (int slot = 0; slot < 140; ++slot)
            EXPECT_EQ(std::memcmp(&live.items[slot], records[index].data() + 4 + slot * 28, 24), 0)
                << "chest " << index << ", slot " << slot;
        const auto grid = mm6::decodeChestGridWire(GameTitle::MM6, records[index]);
        ASSERT_TRUE(grid);
        EXPECT_EQ(live.inventoryMatrix, *grid);
    }

    // A late malformed sidecar must not publish another chest transaction.
    // Restore the valid sidecar before returning to the shared fixture.
    const auto savedSidecar = pOutdoor->mm6DeltaBytes;
    const auto savedState = mm6::chestTitleState();
    const Chest *savedChests = vChests.data();
    pOutdoor->mm6DeltaBytes.pop_back();
    const auto malformed = pOutdoor->mm6DeltaBytes;
    bool accepted = false;
    {
        mm6::ScopedChestMapLoadOptIn optIn;
        accepted = mm6::tryActiveChestMapLoad(GameTitle::MM6, firstMap, pMapTable, pItemTable, pParty);
    }
    const bool rawUntouched = pOutdoor->mm6DeltaBytes == malformed;
    pOutdoor->mm6DeltaBytes = savedSidecar;
    EXPECT_FALSE(accepted);
    EXPECT_TRUE(rawUntouched);
    EXPECT_EQ(vChests.data(), savedChests);
    EXPECT_EQ(mm6::chestTitleState().lcgState, savedState.lcgState);
    EXPECT_EQ(mm6::chestTitleState().generatedArtifactFlags, savedState.generatedArtifactFlags);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
