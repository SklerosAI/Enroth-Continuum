#include <fmt/format.h>

#include <array>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"

#include "Utility/Exception.h"

#include "MM7CombatHarness.h"

namespace {

void toughenPartyForMelee() {
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character.sResBodyBase = 1000;
        character._melee_dmg_bonus = 100;
        character.health = character.GetMaxHealth();
    }
}

bool approachActorForMelee(EngineController &game, MapId map, int actorId) {
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(map, pActors[actorId].pos + offset, yaw, pitch);
            try {
                game.pointMouseAtActor(actorId);
                return true;
            } catch (const Exception &) {
            }
        }
    }
    return false;
}

} // namespace

GAME_TEST(MM7CombatHarness, WineCellarAttackInputDamagesAndKillsTarget) {
    game.startNewGame();
    toughenPartyForMelee();
    pParty->_questBits.set(QBIT_WINE_CELLAR_ACTIVE);
    game.teleportTo(MAP_WINE_CELLAR, Vec3f(601, -512, 1), 0);

    ASSERT_FALSE(pActors.empty());
    ASSERT_EQ(pActors[0].monsterInfo.id, MONSTER_VAMPIRE_C);
    ASSERT_EQ(pActors[0].uniqueNameIndex, 5);
    ASSERT_EQ(pActors[0].aiState, Standing);
    ASSERT_GT(pActors[0].hp, 0);
    ASSERT_GE(pActors.size(), 3u);
    EXPECT_EQ(pActors[1].aiState, Disabled); // Only the authored actor-zero quest target is activated.
    EXPECT_EQ(pActors[2].aiState, Disabled);

    mm7::testing::CombatResult result = mm7::testing::fightActorWithAttackInput(
        game, 0, [&] { return approachActorForMelee(game, MAP_WINE_CELLAR, 0); },
        400, 5, 5);
    const std::string diagnostics = mm7::testing::describeCombatResult(result);

    EXPECT_TRUE(result.targetAcquired) << diagnostics;
    EXPECT_FALSE(result.targetingFailed) << diagnostics;
    EXPECT_TRUE(result.observedHpDecrease) << diagnostics;
    EXPECT_TRUE(result.reachedTerminalDeath) << diagnostics;
    EXPECT_TRUE(result.reachedDeadState) << diagnostics;
    EXPECT_EQ(result.gameTitle, GameTitle::MM7) << diagnostics;
    EXPECT_EQ(result.initialMap, MAP_WINE_CELLAR) << diagnostics;
    EXPECT_EQ(result.finalMap, MAP_WINE_CELLAR) << diagnostics;
    EXPECT_EQ(result.initialScreen, SCREEN_GAME) << diagnostics;
    EXPECT_EQ(result.finalScreen, SCREEN_GAME) << diagnostics;
}

GAME_TEST(MM7CombatHarness, CompletedWineCellarTargetStaysDisabled) {
    for (QuestBit blocker : {QBIT_WINE_CELLAR_VAMPIRE_KILLED, QBIT_WINE_CELLAR_FINISHED}) {
        SCOPED_TRACE(fmt::format("blocker={}", std::to_underlying(blocker)));
        test.prepareForNextTest();
        game.startNewGame();
        pParty->_questBits.set(QBIT_WINE_CELLAR_ACTIVE);
        pParty->_questBits.set(blocker);
        game.teleportTo(MAP_WINE_CELLAR, Vec3f(601, -512, 1), 0);

        ASSERT_FALSE(pActors.empty());
        ASSERT_EQ(pActors[0].uniqueNameIndex, 5);
        EXPECT_EQ(pActors[0].aiState, Disabled);
        EXPECT_TRUE(pActors[0].attributes & ACTOR_UNKNOW11);
    }
}
