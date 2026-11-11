#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/SpriteEnumFunctions.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"

#include "GUI/UI/UIChest.h"

#include "Utility/Memory/Blob.h"

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void walkOverEdge(EngineController &game, MapId map, Vec3f pos, int yaw) {
    game.teleportTo(map, pos, yaw); // Next to the edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterOutd3(EngineController &game) {
    startNewGameThroughInput(game);
    walkOverEdge(game, pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // The west edge.
}

static void clickOutd3Chest4(EngineController &game) {
    // Every chest on outd3 is trapped. The party clicks the near side of the crate that holds chest 4.
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(21632, -17200, 0), 90); // South of the crate, facing it.
    game.pointMouseAtFace(2308);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static std::vector<int> chestContents(int chestId) {
    // The chest's flags, then every item with its grid cell.
    std::vector<int> result = {std::to_underlying(vChests[chestId].flags)};
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        result.insert(result.end(), {std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y});
    return result;
}

static int trapSprites() {
    return std::ranges::count_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.uObjectDescID && object.spriteId >= SPRITE_TRAP_FIRE && object.spriteId <= SPRITE_TRAP_BODY;
    });
}

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

    // Open one real untrapped first-map chest through the public entry point.
    // This must publish the MM6 raw sidecar and live chest together, without
    // entering the inherited MM7 placement path.
    int openIndex = -1;
    for (int index = 0; index < static_cast<int>(vChests.size()); ++index) {
        if (!vChests[index].Trapped() && !vChests[index].Initialized()) {
            openIndex = index;
            break;
        }
    }
    ASSERT_GE(openIndex, 0);
    const auto beforeOpenSidecar = pOutdoor->mm6DeltaBytes;
    bool opened = false;
    game.runGameRoutine([openIndex, &opened] {
        pParty->setActiveCharacterIndex(0);
        opened = Chest::open(openIndex, Pid::dummy());
        if (opened && pGUIWindow_CurrentChest)
            pGUIWindow_CurrentChest->Update();
    });
    EXPECT_TRUE(opened);
    EXPECT_NE(pGUIWindow_CurrentChest, nullptr);
    if (pGUIWindow_CurrentChest)
        EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), openIndex);
    EXPECT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_TRUE(gameTimer->isPaused());
    EXPECT_TRUE(vChests[openIndex].Initialized());
    EXPECT_NE(pOutdoor->mm6DeltaBytes, beforeOpenSidecar);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    game.runGameRoutine([] {
        pGUIWindow_CurrentChest.reset();
        current_screen_type = SCREEN_GAME;
        gameTimer->setPaused(false);
    });
}

GAME_TEST(MM6ChestTrap, FailedDisarmSetsOffTrap) {
    // Trapped MM6 chests never opened. A party that fails the disarm sets the trap off with the first click, and the
    // second click opens the chest.
    enterOutd3(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // A new party dies to the trap.
    auto hpTape = charTapes.hps();
    auto chestTape = tapes.custom([] { return chestContents(4); });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    auto soundsTape = tapes.sounds();
    test.startTaping();

    clickOutd3Chest4(game);
    game.tick(7); // The trap's flash burns out and hits the party.
    game.pointMouseAtFace(2308);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
    EXPECT_EQ(trapTape, tape(0, 1, 0));
    ASSERT_EQ(hpTape.size(), 2);
    for (int hp : hpTape.back())
        EXPECT_LT(hp, 200);
    ASSERT_EQ(chestTape.size(), 2);
    EXPECT_EQ(chestTape.front().front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape.back().front(), std::to_underlying(CHEST_ITEMS_PLACED)); // The trap is gone.
    EXPECT_GT(chestTape.back().size(), 1);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_fireBall);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_openchest0101);
}

GAME_TEST(MM6ChestTrap, DisarmedTrapOpensChest) {
    // Trapped MM6 chests never opened. A character good enough at disarming opens one with the first click and no harm.
    enterOutd3(game);
    pParty->activeCharacter().setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(6, MASTERY_NOVICE)); // Beats outd3's lock even on the worst roll.
    auto hpTape = charTapes.hps();
    auto chestTape = tapes.custom([] { return chestContents(4); });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    auto soundsTape = tapes.sounds();
    test.startTaping();

    clickOutd3Chest4(game);

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
    EXPECT_EQ(trapTape, tape(0));
    EXPECT_EQ(hpTape.size(), 1);
    ASSERT_EQ(chestTape.size(), 2);
    EXPECT_EQ(chestTape.front().front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape.back().front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_GT(chestTape.back().size(), 1);
    EXPECT_MISSES(soundsTape.flatten(), SOUND_fireBall);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_openchest0101);
}

GAME_TEST(MM6ChestTrap, SprungTrapStaysGoneAfterReturn) {
    // A chest whose trap went off stays untrapped with its items when the party leaves outd3 and comes back.
    enterOutd3(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // A new party dies to the trap.
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(chestContents(4)) : std::nullopt; });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    test.startTaping();

    clickOutd3Chest4(game);
    game.tick(7); // The trap's flash burns out and hits the party.
    walkOverEdge(game, outd3, Vec3f(22450, -10000, 164), 0); // The east edge.
    walkOverEdge(game, pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // The west edge.
    clickOutd3Chest4(game);

    EXPECT_EQ(mapTape, tape(outd3, pMapTable->GetMapInfo("oute3.odm"), outd3));
    EXPECT_EQ(screenTape.back(), SCREEN_CHEST);
    EXPECT_EQ(trapTape.max(), 1); // Only the first click sets a trap off.
    ASSERT_EQ(chestTape.size(), 4);
    EXPECT_EQ(chestTape[0]->front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape[1]->front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_EQ(chestTape[2], std::nullopt);
    EXPECT_EQ(chestTape[3], chestTape[1]);
}
