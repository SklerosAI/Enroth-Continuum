#include <cstdint>
#include <optional>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MM6/MM6ChestBaseItemPicker.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

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

static int weakCharacters() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.conditions.has(CONDITION_WEAK);
    return result;
}

static std::vector<int> chestContents(int chestId) {
    // The chest's flags, then every item with its grid cell.
    std::vector<int> result = {std::to_underlying(vChests[chestId].flags)};
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        result.insert(result.end(), {std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y});
    return result;
}

static Actor *spawnPeasantByTheD02Door(EngineController &game) {
    // A weak monster in front of the party, on open ground by the d02 door.
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21000, -1500, 160), 0);
    return game.spawnMonster(Vec3f(-20850, -1500, 160), static_cast<MonsterId>(121), SPAWN_DUMMY);
}

static void killAndLootChest(EngineController &game, const Actor *monster) {
    // The party hits the monster in front of it until it's dead, then opens an untrapped chest and takes its first item
    // that isn't gold.
    game.pointMouseAtActor(monster->id);
    for (int attack = 0; attack < 1000 && monster->aiState != Dead; attack++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A chest opens only for a character who has recovered from the fight.
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-19696, -9686, 0), 270); // In front of the chest, facing it.
    game.pointMouseAtFace(897); // The chest's near side.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    for (InventoryEntry entry : vChests[2].inventory.entries()) {
        if (entry->isGold())
            continue;
        game.pressAndReleaseButton(BUTTON_LEFT, Pointi(42, 34) + entry.geometry().topLeft() * 32 + Pointi(16, 16)); // The chest grid of type 0.
        game.tick();
        break;
    }
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.moveMouse(600, 400); // Off the viewport. Hovering an interactive decoration without a variable of its own indexes the decoration variables at -75.
    game.tick();
}

static void confirmTransition(EngineController &game) {
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6MapTravel, WalkOffEdgeToNeighbour) {
    // Walking off an MM6 outdoor map only pushed the party back, no neighbouring map was ever offered.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto foodTape = tapes.food();
    auto weakTape = tapes.custom([] { return weakCharacters(); });
    game.teleportTo(oute3, Vec3f(-22450, -10000, 100), 180); // Next to the west edge, facing it.
    test.startTaping();

    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_EQ(screenTape.back(), SCREEN_CHANGE_LOCATION);
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(positionTape, Vec3f(-22606, -10000, 110)); // Past the edge when the dialogue opened.
    EXPECT_CONTAINS(positionTape, Vec3f(22450, -10000, 164)); // One map width east, on the terrain of the new map.
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), Duration::fromDays(5).ticks()); // The trip, in one frame.
    EXPECT_EQ(foodTape, tape(7, 2));
    EXPECT_EQ(weakTape, tape(0));
}

GAME_TEST(MM6MapTravel, DungeonDoorAndBack) {
    // An MM6 dungeon door offered only Cancel, the party couldn't enter the dungeon or leave it.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto foodTape = tapes.food();
    game.teleportTo(oute3, Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_INPUT_BLV);
    confirmTransition(game);
    ASSERT_EQ(mapTape.back(), d02);
    EXPECT_EQ(positionTape.back(), Vec3f(16406, -19669, 864.5f)); // Where the door's script puts the party.

    game.teleportTo(d02, Vec3f(16384, -19626, 864.5f), 270); // In front of the way out, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_INPUT_BLV);
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(oute3, d02, oute3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(positionTape.back(), Vec3f(-21468, -263, 193)); // Back in front of the door, where the way out's script puts it.
    EXPECT_EQ(foodTape, tape(7));
}

GAME_TEST(MM6MapTravel, DungeonAndBackKeepsTheMap) {
    // An MM6 map came back as it was saved when the party returned to it, a killed monster stood again and a looted
    // chest was refilled.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    const Actor *peasant = spawnPeasantByTheD02Door(game);
    const int peasantId = peasant->id;
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto peasantTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == oute3 && peasantId < std::ssize(pActors) ? std::optional(pActors[peasantId].aiState) : std::nullopt;
    });
    auto chestTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == oute3 ? std::optional(chestContents(2)) : std::nullopt;
    });
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto chestRandomTape = tapes.custom([] { return mm6::chestTitleState().lcgState; });
    test.startTaping();

    killAndLootChest(game, peasant);
    game.teleportTo(oute3, Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
    game.teleportTo(d02, Vec3f(16384, -19626, 864.5f), 270); // In front of the way out, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(oute3, d02, oute3));
    EXPECT_CONTAINS(screenTape, SCREEN_CHEST);
    EXPECT_CONTAINS(peasantTape, Dead);
    EXPECT_EQ(peasantTape.back(), Dead);
    ASSERT_EQ(chestTape.size(), 5);
    EXPECT_NE(chestTape[0], chestTape[1]); // The first opening places the items.
    EXPECT_EQ(chestTape[2]->size(), chestTape[1]->size() - 3); // One item fewer.
    EXPECT_EQ(chestTape[3], std::nullopt);
    EXPECT_EQ(chestTape[4], chestTape[2]);
    EXPECT_EQ(heldTape.size(), 2);
    std::uint32_t lastLoadDraw = chestRandomTape[chestRandomTape.size() - 2];
    mm6::nextChestRandom15(lastLoadDraw);
    EXPECT_EQ(chestRandomTape.back(), lastLoadDraw); // The chest pass on the way back finds no random items left to make.
}

GAME_TEST(MM6MapTravel, NeighbourAndBackKeepsTheMap) {
    // Coming back over the edge from a neighbouring MM6 map brought back the map as it was saved, a killed monster stood
    // again and a looted chest was refilled.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const Actor *peasant = spawnPeasantByTheD02Door(game);
    const int peasantId = peasant->id;
    auto mapTape = tapes.map();
    auto peasantTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == oute3 && peasantId < std::ssize(pActors) ? std::optional(pActors[peasantId].aiState) : std::nullopt;
    });
    auto chestTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == oute3 ? std::optional(chestContents(2)) : std::nullopt;
    });
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    test.startTaping();

    killAndLootChest(game, peasant);
    game.teleportTo(oute3, Vec3f(-22450, -10000, 100), 180); // Next to the west edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);
    game.teleportTo(outd3, Vec3f(22450, -10000, 164), 0); // Back at the east edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3, oute3));
    EXPECT_CONTAINS(peasantTape, Dead);
    EXPECT_EQ(peasantTape.back(), Dead);
    ASSERT_EQ(chestTape.size(), 5);
    EXPECT_NE(chestTape[0], chestTape[1]); // The first opening places the items.
    EXPECT_EQ(chestTape[2]->size(), chestTape[1]->size() - 3); // One item fewer.
    EXPECT_EQ(chestTape[3], std::nullopt);
    EXPECT_EQ(chestTape[4], chestTape[2]);
    EXPECT_EQ(heldTape.size(), 2);
}
