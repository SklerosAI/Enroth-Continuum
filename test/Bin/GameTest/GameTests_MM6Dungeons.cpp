#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIRest.h"

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

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterD02(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static std::vector<int> monstersNearParty(float range) {
    std::vector<int> result;
    for (Actor &actor : pActors)
        if (!actor.IsNotAlive() && (actor.pos - pParty->pos).length() < range)
            result.push_back(actor.id);
    return result;
}

static bool partyHurt() {
    return std::ranges::any_of(pParty->pCharacters, [](const Character &character) { return character.health < character.GetMaxHealth(); });
}

static bool standAwayFrom(EngineController &game, Vec3f target, float distance) {
    // On the floor of the target's sector, the given distance away from it and facing it. False when there's no floor.
    int targetSector = 0;
    GetIndoorFloorZ(target + Vec3f(0, 0, 40), &targetSector);
    for (Vec3f direction : {Vec3f(0, -1, 0), Vec3f(-1, 0, 0), Vec3f(0, 1, 0), Vec3f(1, 0, 0)}) {
        Vec3f position = target + distance * direction;
        int sectorId = 0;
        position.z = GetIndoorFloorZ(position + Vec3f(0, 0, 40), &sectorId);
        if (sectorId != targetSector)
            continue;
        game.teleportTo(engine->_currentLoadedMapId, position, std::lround(std::atan2(-direction.y, -direction.x) * 180 / std::numbers::pi));
        return true;
    }
    return false;
}

static void restAndHeal(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_REST);
    game.pressGuiButton("Rest_RestAndHeal");
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick();
}

GAME_TEST(MM6Dungeons, TempleBatsBiteTheParty) {
    // The Abandoned Temple's monsters never noticed the party. The bats of the entrance hall fly up and bite.
    startNewGameThroughInput(game);
    enterD02(game);
    auto hpTape = tapes.totalHp();
    auto huntersTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.monsterInfo.hostilityType == HOSTILITY_LONG; });
    });
    test.startTaping();

    game.tick(100); // The bats fly up and bite.

    EXPECT_LT(hpTape.delta(), 0);
    EXPECT_GT(huntersTape.back(), 0);
}

GAME_TEST(MM6Dungeons, RestNearAndFarFromMonsters) {
    // The Abandoned Temple's monsters never noticed the party, and the party could rest next to them. Now the bats of
    // the entrance hall bite and the party can't rest there. In the cage room, far from every monster, it rests and
    // heals.
    startNewGameThroughInput(game);
    enterD02(game);
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto timeTape = tapes.time();
    auto hpTape = charTapes.hps();
    auto conditionTape = charTapes.conditions();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    EXPECT_EQ(statusTape.back(), localization->str(LSTR_THERE_ARE_HOSTILE_ENEMIES_NEAR));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    for (int frame = 0; frame < 300 && !partyHurt(); frame++)
        game.tick(); // The bats fly up and bite.
    ASSERT_TRUE(partyHurt());

    ASSERT_TRUE(standAwayFrom(game, pLevelDecorations[106].vPosition, 150)); // In the cage room, by the first cage.
    ASSERT_TRUE(monstersNearParty(2560).empty());
    restAndHeal(game);

    EXPECT_CONTAINS(screenTape, SCREEN_REST);
    EXPECT_TRUE(std::ranges::any_of(hpTape, [&](const auto &hps) { return hps != hpTape.front(); })); // The bats hurt somebody.
    EXPECT_EQ(hpTape.back(), hpTape.front()); // And the rest healed everyone.
    EXPECT_EQ(conditionTape.back(), tape(CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD));
    EXPECT_GE(timeTape.back() - timeTape.front(), Duration::fromHours(8));
}
