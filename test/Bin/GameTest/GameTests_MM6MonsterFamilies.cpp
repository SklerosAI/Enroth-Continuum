#include <limits>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"

#include "Utility/Math/TrigLut.h"

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

static void killByInput(EngineController &game, int actorId) {
    // The party swings at the monster until it falls. Blows can miss.
    for (int attack = 0; attack < 1000 && !pActors[actorId].IsNotAlive(); attack++) {
        game.pointMouseAtActor(actorId);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
}

static int nearestActor(Vec3f pos, int monster) {
    // The map's actor of this MM6 monster ID nearest to the given point.
    int result = -1;
    float bestDistance = std::numeric_limits<float>::max();
    for (const Actor &actor : pActors) {
        const float distance = (actor.pos - pos).length();
        if (actor.monsterInfo.id == static_cast<MonsterId>(monster) && distance < bestDistance) {
            bestDistance = distance;
            result = actor.id;
        }
    }
    return result;
}

static Vec3f nearestShoreWater() {
    // Center of the water tile nearest to the party that has land on the next two tiles to the north.
    Vec3f result;
    float bestDistance = std::numeric_limits<float>::max();
    for (int y = 2; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            if (!pOutdoor->pTerrain.isWaterByGrid({x, y}) || pOutdoor->pTerrain.isWaterByGrid({x, y - 1}) ||
                pOutdoor->pTerrain.isWaterByGrid({x, y - 2}))
                continue;
            const Vec3f center((x - 64) * 512 + 256, (63 - y) * 512 + 256, 0);
            const float distance = (center - pParty->pos).xy().length();
            if (distance < bestDistance) {
                bestDistance = distance;
                result = center;
            }
        }
    }
    result.z = pOutdoor->pTerrain.heightByPos(result);
    return result;
}

GAME_TEST(MM6MonsterFamilies, KillingFinesNobody) {
    // Killing a rat fined the party and gave every character award 1, Returned the Prince, because MM7's peasant types
    // cover MM6's rats. MM6 has no fines, a dead peasant costs 100 reputation instead.
    for (bool peasant : {false, true}) {
        SCOPED_TRACE(peasant);
        game.goToMainMenu();
        startNewGameThroughInput(game);
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const MonsterId monster = static_cast<MonsterId>(peasant ? 121 : 145); // Peasant or Common Rat.
        const int victim = game.spawnMonster(pParty->pos + ahead * 150, monster, SPAWN_DUMMY)->id;
        auto stateTape = actorTapes.aiState(victim);
        auto goldTape = tapes.gold();
        auto fineTape = tapes.custom([] { return pParty->uFine; });
        auto awardTape = charTapes.custom([](const Character &character) { return character._achievedAwardsBits[AWARD_FINE]; });
        auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
        test.startTaping();

        killByInput(game, victim);
        test.stopTaping();

        EXPECT_CONTAINS(stateTape, Dying);
        EXPECT_EQ(goldTape, tape(200));
        EXPECT_EQ(fineTape, tape(0));
        EXPECT_EQ(awardTape, tape({false, false, false, false}));
        if (peasant) {
            EXPECT_EQ(reputationTape, tape(0, -100));
        } else {
            EXPECT_EQ(reputationTape, tape(0));
        }
    }
}

GAME_TEST(MM6MonsterFamilies, HurtingAPeasantCostsReputation) {
    // Hurting and killing a peasant cost no reputation. MM6 takes 50 for the first blow at a peasant, who wasn't
    // hostile, and 100 more when she dies. A friendly rat costs nothing, its kind is hostile.
    for (bool peasant : {true, false}) {
        SCOPED_TRACE(peasant);
        game.goToMainMenu();
        startNewGameThroughInput(game);
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const MonsterId monster = static_cast<MonsterId>(peasant ? 123 : 145); // Peasant or Common Rat.
        const int victim = game.spawnMonster(pParty->pos + ahead * 150, monster, SPAWN_STATIONARY | SPAWN_FRIENDLY)->id;
        auto hpTape = actorTapes.hp(victim);
        auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
        test.startTaping();

        killByInput(game, victim);
        test.stopTaping();

        EXPECT_GT(hpTape.size(), 2); // More than one blow.
        if (peasant) {
            EXPECT_EQ(reputationTape, tape(0, -50, -150));
        } else {
            EXPECT_EQ(reputationTape, tape(0));
        }
    }
}

GAME_TEST(MM6MonsterFamilies, SwordsmenDropNoReagent) {
    // Swordsmen left a Lightning Bolt scroll on the ground on one death in five, MM7's ooze reagent, because MM7's ooze
    // type covers MM6's swordsmen. MM6 has no reagent drops.
    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const Vec3f side(-ahead.y, ahead.x, 0);
    auto itemsTape = tapes.mapItemCount();
    auto experienceTape = tapes.totalExperience();
    test.startTaping();

    for (int spot = 0; spot < 10; spot++) { // Each swordsman stands clear of the bodies of the others.
        const Vec3f position = pParty->pos + ahead * (spot < 5 ? 250 : 350) + side * ((spot % 5 - 2) * 90 + (spot < 5 ? 0 : 45));
        Actor *swordsman = game.spawnMonster(position, static_cast<MonsterId>(112), SPAWN_DUMMY);
        swordsman->hp = 1; // One blow kills him, a new party barely scratches a swordsman.
        killByInput(game, swordsman->id);
    }

    EXPECT_EQ(experienceTape.size(), 11); // Ten swordsmen died.
    EXPECT_EQ(itemsTape, tape(42)); // What lies about New Sorpigal from the start.
}

GAME_TEST(MM6MonsterFamilies, PeasantsStandTogether) {
    // Killing a woman in New Sorpigal left the men next to her friendly, and killing a rat turned them hostile, because
    // MM7's peasant races split MM6's families. MM6 turns all three peasant families hostile when one of them is hurt,
    // and only its own family for any other monster.
    for (bool peasant : {true, false}) {
        SCOPED_TRACE(peasant);
        game.goToMainMenu();
        startNewGameThroughInput(game);
        const int woman = nearestActor(pParty->pos, 121);
        const int man = nearestActor(pActors[woman].pos, 134);
        const int victim = peasant ? woman : game.spawnMonster(pActors[man].pos + Vec3f(300, 0, 0), static_cast<MonsterId>(145), SPAWN_DUMMY)->id;
        game.teleportTo(engine->_currentLoadedMapId, pActors[victim].pos - Vec3f(150, 0, 0), 0); // West of the victim, facing it.
        auto victimTape = actorTapes.aiState(victim);
        auto hostilityTape = actorTapes.custom({woman, man}, [](const Actor &actor) { return actor.monsterInfo.hostilityType; });
        auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
        test.startTaping();

        killByInput(game, victim);
        test.stopTaping();

        EXPECT_CONTAINS(victimTape, Dying);
        if (peasant) {
            EXPECT_EQ(hostilityTape, tape({HOSTILITY_FRIENDLY, HOSTILITY_FRIENDLY}, {HOSTILITY_FRIENDLY, HOSTILITY_LONG}));
            EXPECT_EQ(reputationTape, tape(0, -100));
        } else {
            EXPECT_EQ(hostilityTape, tape({HOSTILITY_FRIENDLY, HOSTILITY_FRIENDLY}));
            EXPECT_EQ(reputationTape, tape(0));
        }
    }
}

GAME_TEST(MM6MonsterFamilies, SeaMonstersStayInWater) {
    // Sea Serpents and Water Elementals swam ashore, because MM7's water elemental type covers MM6's dwarves instead.
    // MM6 keeps both families in the water, they turn back at the shore.
    startNewGameThroughInput(game);
    const Vec3f shore = nearestShoreWater();
    game.teleportTo(engine->_currentLoadedMapId, shore + Vec3f(0, 1024, 0), 270); // Two tiles inland, facing the water.
    const int serpent = game.spawnMonster(shore - Vec3f(120, 0, 0), static_cast<MonsterId>(151), SPAWN_LEVEL_1)->id;
    const int beast = game.spawnMonster(shore + Vec3f(120, 0, 0), static_cast<MonsterId>(58), SPAWN_LEVEL_1)->id;
    auto waterTape = actorTapes.custom({serpent, beast}, [](const Actor &actor) { return pOutdoor->pTerrain.isWaterByPos(actor.pos); });
    auto distanceTape = actorTapes.custom({serpent, beast}, [](const Actor &actor) { return static_cast<int>((actor.pos - pParty->pos).xy().length()) / 128; });
    test.startTaping();

    game.tick(100); // The serpent and the water beast come for the party.

    EXPECT_EQ(waterTape, tape({true, true}));
    EXPECT_GT(distanceTape.size(), 1);
}
