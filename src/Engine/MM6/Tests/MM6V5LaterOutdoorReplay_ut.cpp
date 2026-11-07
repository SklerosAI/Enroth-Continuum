#include <array>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V5OutdoorMapIdentity.h"
#include "Engine/MM6/MM6V5PreEventActorReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventDecorationReplayForTest.h"

UNIT_TEST(MM6V5LaterOutdoorReplay, AcceptsExactlyShippedOutdoorGridNames) {
    int accepted = 0;
    for (char column = 'a'; column <= 'e'; ++column)
        for (char row = '1'; row <= '3'; ++row) {
            std::string name = "out";
            name += column;
            name += row;
            name += ".odm";
            accepted += mm6::isMM6OutdoorMapName(name);
        }
    EXPECT_EQ(accepted, 15);
    EXPECT_FALSE(mm6::isMM6OutdoorMapName("outf1.odm"));
    EXPECT_FALSE(mm6::isMM6OutdoorMapName("outa4.odm"));
    EXPECT_FALSE(mm6::isMM6OutdoorMapName("outa1.blv"));
    EXPECT_FALSE(mm6::isMM6OutdoorMapName("OUTA1.ODM"));
}

UNIT_TEST(MM6V5LaterOutdoorReplay, RestoresLaterOutdoorActorWithExactMapBinding) {
    Actor actor(0);
    actor.monsterInfo.id = MONSTER_GOBLIN_A;
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    const std::array<Actor, 1> baseline{actor};
    const auto baselineBytes = mm6::encodeActiveActorSection(baseline, GameTitle::MM6);
    Actor moved = actor;
    moved.pos = Vec3f(1.25f, -2.5f, 3.75f);
    const std::array<Actor, 1> action{moved};
    mm6::testing::ScopedV5PreEventActorReplayForTest scope(
        GameTitle::MM6,
        mm6::captureActorRuntimeFields("oute2.odm", baselineBytes, action, GameTitle::MM6),
        baselineBytes);
    std::deque<Actor> live{actor};
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, true,
        "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, true,
        "oute2.odm", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(live[0].pos, moved.pos);
}

UNIT_TEST(MM6V5LaterOutdoorReplay, RestoresLaterOutdoorDecorationOnlyAfterEvents) {
    mm6::DecorationRuntimeSaveFields fields;
    fields.mapName = "oute2.odm";
    LevelDecoration decoration;
    decoration.uDecorationDescID = DECORATION_CAMPFIRE_5;
    decoration.uFlags |= LEVEL_DECORATION_VISIBLE_ON_MAP;
    fields.decorations.push_back(decoration);
    mm6::testing::ScopedV5PreEventDecorationReplayForTest scope(
        GameTitle::MM6, fields);
    std::vector<LevelDecoration> rebuilt(1);
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        true, false, "oute2.odm", GameTitle::MM6), std::exception);
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        true, true, "oute2.odm", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(rebuilt[0].uFlags, decoration.uFlags);
}
