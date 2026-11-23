#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "MM6MapEntranceForTest.h"

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

static void enterD02(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static std::vector<int> encounterGroup(Vec3f spawn, int radius) {
    // The monsters the map's spawn point at this position put down, on it or on the ring around it.
    std::vector<int> result;
    for (const Actor &actor : pActors)
        if ((actor.initialPosition - spawn).xy().length() < radius + 2)
            result.push_back(actor.id);
    return result;
}

template<class Callback>
static auto groupTape(CommonTapeRecorder &tapes, const std::vector<int> &group, Callback callback) {
    // What the callback says about each monster of the group.
    return tapes.custom([group, callback] {
        std::vector<decltype(callback(pActors[0]))> result;
        for (int id : group)
            result.push_back(callback(pActors[id]));
        return result;
    });
}

static int ringDistance(const Actor &actor, Vec3f spawn) {
    return std::lround((actor.initialPosition - spawn).xy().length());
}

GAME_TEST(MM6Encounters, OutdoorGroupStandsInARing) {
    // New Sorpigal's goblins stood stacked on their spawn point and started out friendly. MM6 puts every goblin after the
    // first on a ring 128 around it, hostile as monsters.txt says.
    startNewGameThroughInput(game);
    const Vec3f spawn(-1104, -10816, 0); // A goblin spawn point east of the party.
    const std::vector<int> goblins = encounterGroup(spawn, 128);
    const int peasant = std::ranges::min(pActors, {}, [](const Actor &actor) {
        return actor.monsterInfo.id == static_cast<MonsterId>(121) ? (actor.pos - pParty->pos).length() : 1e9f;
    }).id;
    auto distanceTape = groupTape(tapes, goblins, [spawn](const Actor &actor) { return ringDistance(actor, spawn); });
    auto monsterTape = groupTape(tapes, goblins, [](const Actor &actor) { return actor.monsterInfo.id; });
    auto hostilityTape = groupTape(tapes, goblins, [](const Actor &actor) { return actor.monsterInfo.hostilityType; });
    auto peasantTape = actorTapes.custom(peasant, [](const Actor &actor) { return actor.monsterInfo.hostilityType; });
    test.startTaping();

    game.tick();

    constexpr MonsterId goblin = static_cast<MonsterId>(76);
    EXPECT_EQ(distanceTape, tape(std::vector<int>{0, 128, 128, 129})); // MM6 rounds a ring point's coordinates down.
    EXPECT_EQ(monsterTape, tape(std::vector<MonsterId>(4, goblin)));
    EXPECT_EQ(hostilityTape, tape(std::vector<MonsterHostility>(4, HOSTILITY_LONG)));
    EXPECT_EQ(peasantTape, tape(HOSTILITY_FRIENDLY));
}

GAME_TEST(MM6Encounters, IndoorGroupStandsInARing) {
    // The Abandoned Temple's cobras stood stacked on their spawn point and started out friendly. MM6 puts every cobra
    // after the first on a ring 64 around it, hostile as monsters.txt says. A ring point in another sector puts the
    // monster on the spawn point instead, as with the bats in the doorway of the entrance hall.
    startNewGameThroughInput(game);
    enterD02(game);
    const Vec3f cobraSpawn(17568, -15872, 752);
    const Vec3f batSpawn(16384, -17440, 752);
    const std::vector<int> cobras = encounterGroup(cobraSpawn, 64);
    const std::vector<int> bats = encounterGroup(batSpawn, 64);
    auto cobraDistanceTape = groupTape(tapes, cobras, [cobraSpawn](const Actor &actor) { return ringDistance(actor, cobraSpawn); });
    auto cobraHostilityTape = groupTape(tapes, cobras, [](const Actor &actor) { return actor.monsterInfo.hostilityType; });
    auto batDistanceTape = groupTape(tapes, bats, [batSpawn](const Actor &actor) { return ringDistance(actor, batSpawn); });
    auto batSectorTape = groupTape(tapes, bats, [](const Actor &actor) { return pIndoor->GetSector(actor.initialPosition); });
    test.startTaping();

    game.tick();

    EXPECT_EQ(cobraDistanceTape, tape(std::vector<int>{0, 64, 65, 64, 63}));
    EXPECT_EQ(cobraHostilityTape, tape(std::vector<MonsterHostility>(5, HOSTILITY_LONG)));
    EXPECT_EQ(batDistanceTape, tape(std::vector<int>{0, 0, 65, 0}));
    EXPECT_EQ(batSectorTape, tape(std::vector<int>{44, 44, 44, 44}));
}

GAME_TEST(MM6Encounters, LevelOddsStopAtFour) {
    // The Free Haven sewer's rats came out 10% small, 50% medium and 40% large, as MM7's top level odds. MM6 caps a
    // map's level odds at its fourth row, 30%, 40% and 30%.
    startNewGameThroughInput(game);
    const std::optional<MM6ShippedEntrance> entrance = findShippedMM6Entrance("sewer.blv");
    ASSERT_TRUE(entrance);
    game.teleportTo(pMapTable->GetMapInfo("sewer.blv"), entrance->placement.pos, 0); // Where the sewer door puts the party.
    auto ratsTape = tapes.custom([] {
        std::vector<int> result(3);
        for (const Actor &actor : pActors)
            if (actor.monsterInfo.id >= static_cast<MonsterId>(145) && actor.monsterInfo.id <= static_cast<MonsterId>(147))
                result[std::to_underlying(actor.monsterInfo.id) - 145]++;
        return result;
    });
    test.startTaping();

    game.tick();

    EXPECT_EQ(ratsTape, tape(std::vector<int>{31, 39, 31}));
}
