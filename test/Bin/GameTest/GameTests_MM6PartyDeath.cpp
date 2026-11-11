#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
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

static void enterDungeonDoor(EngineController &game) {
    // The d02 door on the first map, its transition confirmed.
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static std::vector<int> spawnKillers(EngineController &game) {
    // Four monsters that kill a new character with one blow, around the party.
    std::vector<int> result;
    for (Vec3f offset : {Vec3f(0, 200, 0), Vec3f(0, -200, 0), Vec3f(200, 0, 0), Vec3f(-200, 0, 0)}) {
        Actor *killer = game.spawnMonster(pParty->pos + offset, static_cast<MonsterId>(108), SPAWN_STATIONARY);
        killer->monsterInfo.spell1UseChance = 0; // Blows only, its Finger of Death would kill from afar.
        result.push_back(killer->id);
    }
    return result;
}

static void walkForward(EngineController &game) {
    game.pressKey(PlatformKey::KEY_UP);
    game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
}

GAME_TEST(MM6PartyDeath, DiesInDungeonWakesInNewSorpigal) {
    // A dead MM6 party was sent to MM7's Emerald Island, where loading the chests refused.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto deathsTape = tapes.deaths();
    auto goldTape = tapes.gold();
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto statusTape = tapes.statusBar();
    auto positionTape = tapes.custom([] { return pParty->pos.xy(); });
    auto hpTape = charTapes.hps();
    auto conditionTape = charTapes.conditions();
    auto awardTape = charTapes.custom([](const Character &character) { return character._achievedAwardsBits.test(static_cast<AwardId>(82)); });
    test.startTaping();

    enterDungeonDoor(game);
    ASSERT_EQ(mapTape.back(), d02);
    spawnKillers(game);
    game.skipLoadingScreen(); // The killers walk up and strike everyone down, the losing movie plays and New Sorpigal loads.
    walkForward(game);

    EXPECT_EQ(mapTape, tape(oute3, d02, oute3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(deathsTape, tape(0, 1));
    EXPECT_EQ(goldTape, tape(200, 0));
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), Duration::fromDays(7).ticks() + 12); // A week, and the frame the party died in.
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_ONCE_AGAIN_YOUVE_CHEATED_DEATH));
    EXPECT_CONTAINS(positionTape, Vec2f(-9728, -11319)); // By the New Sorpigal fountain.
    EXPECT_NE(positionTape.back(), Vec2f(-9728, -11319)); // And walking away from it.
    EXPECT_EQ(hpTape.back(), tape(1, 1, 1, 1));
    EXPECT_EQ(conditionTape.back(), tape(CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD));
    EXPECT_EQ(awardTape.frontBack(), tape({false, false, false, false}, {true, true, true, true}));
}

GAME_TEST(MM6PartyDeath, DiesOnFirstMapWakesWithoutReload) {
    // A dead MM6 party on the first map was sent to MM7's Emerald Island too.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    game.teleportTo(oute3, Vec3f(-21456, -286, 193), 90); // Far from the fountain.
    const std::vector<int> killers = spawnKillers(game);
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto deathsTape = tapes.deaths();
    auto positionTape = tapes.custom([] { return pParty->pos.xy(); });
    auto hpTape = charTapes.hps();
    auto hostilityTape = tapes.custom([&] { return pActors[killers[0]].monsterInfo.hostilityType; });
    test.startTaping();

    game.tick(64); // The killers walk up and strike everyone down, and the losing movie plays.
    walkForward(game);

    EXPECT_EQ(mapTape, tape(oute3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(deathsTape, tape(0, 1));
    EXPECT_CONTAINS(positionTape, Vec2f(-9728, -11319));
    EXPECT_NE(positionTape.back(), Vec2f(-9728, -11319));
    EXPECT_EQ(hpTape.back(), tape(1, 1, 1, 1));
    EXPECT_EQ(hostilityTape, tape(HOSTILITY_LONG)); // MM7 makes every monster friendly, MM6 reads the monster table.
}
