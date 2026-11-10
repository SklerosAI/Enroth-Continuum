#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "GUI/GUIWindow.h"

static std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

static bool movesToMap(int eventId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MoveToMap;
    });
}

static bool isXenofex(const Actor &actor) {
    return actor.uniqueNameIndex != 0 && pMonsterStats->uniqueNames[actor.uniqueNameIndex] == "Xenofex";
}

static void strengthenParty() {
    // At level 100 and without speed or endurance the party falls to Xenofex and the devils next to him.
    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character.health = character.GetMaxHealth();
    }
}

static void leaveThroughExit(EngineController &game) {
    auto exit = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && movesToMap(face.eventId);
    });
    ASSERT_NE(exit, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*exit);
    game.teleportTo(MAP_COLONY_ZOD, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
}

template<class T>
static std::optional<T> onColonyZod(T value) {
    return engine->_currentLoadedMapId == MAP_COLONY_ZOD ? std::optional(value) : std::nullopt; // Outside of it the actor list is another map's.
}

GAME_TEST(MM7XenofexCombat, KillAndLeave) {
    // Killing Xenofex in a real fight and leaving Colony Zod completes the kill step, leaving with him alive doesn't.
    game.startNewGame();
    pParty->_questBits.set(QBIT_XENOFEX_LIGHT_ACTIVE);
    strengthenParty();

    for (bool kill : {false, true}) {
        SCOPED_TRACE(kill ? "killed" : "alive");
        game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
        std::vector<int> candidates;
        for (const Actor &actor : pActors)
            if (isXenofex(actor))
                candidates.push_back(actor.id);
        ASSERT_EQ(candidates.size(), 1u);
        const int xenofexId = candidates.front();
        ASSERT_EQ(pActors[xenofexId].monsterInfo.id, MONSTER_DEVIL_C);
        ASSERT_FALSE(pActors[xenofexId].IsNotAlive());

        auto xenofexHpTape = tapes.custom([=] { return onColonyZod<int>(pActors[xenofexId].hp); });
        auto xenofexDeadTape = tapes.custom([=] { return onColonyZod(pActors[xenofexId].IsNotAlive()); });
        auto killedTape = tapes.questBit(QBIT_XENOFEX_KILLED);
        auto mapTape = tapes.map();
        auto partyHpTape = tapes.totalHp();
        test.startTaping();

        if (kill) {
            game.teleportTo(MAP_COLONY_ZOD, pActors[xenofexId].pos + Vec3f(0, -250, 0), 90); // Into melee range, facing Xenofex.
            game.pointMouseAtActor(xenofexId);
            for (int attack = 0; attack < 1000 && !pActors[xenofexId].IsNotAlive(); attack++) {
                game.pressAndReleaseKey(PlatformKey::KEY_A);
                game.tick();
            }
            ASSERT_TRUE(pActors[xenofexId].IsNotAlive()) << "remaining HP: " << pActors[xenofexId].hp;
        }

        leaveThroughExit(game);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
        test.stopTaping();

        EXPECT_EQ(mapTape, tape(MAP_COLONY_ZOD, MAP_LAND_OF_THE_GIANTS));
        EXPECT_TRUE(xenofexHpTape.front().has_value());
        EXPECT_EQ(xenofexHpTape.back(), std::nullopt);
        if (kill) {
            EXPECT_EQ(killedTape, tape(false, true));
            EXPECT_EQ(xenofexDeadTape, tape(std::optional(false), std::optional(true), std::optional<bool>()));
            EXPECT_TRUE(xenofexHpTape.contains([](std::optional<int> hp) { return hp && *hp <= 0; }));
            EXPECT_LT(partyHpTape.back(), partyHpTape.front());
        } else {
            EXPECT_EQ(killedTape, tape(false));
            EXPECT_EQ(xenofexDeadTape, tape(std::optional(false), std::optional<bool>()));
            EXPECT_EQ(xenofexHpTape.size(), 2u);
            EXPECT_EQ(partyHpTape.size(), 1u);
        }
    }
}
