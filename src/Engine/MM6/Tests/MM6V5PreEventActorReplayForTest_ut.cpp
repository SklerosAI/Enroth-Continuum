#include <array>
#include <bit>
#include <cstdint>
#include <deque>
#include <exception>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V5PreEventActorReplayForTest.h"

namespace {

Actor baselineActor() {
    Actor actor(0);
    actor.monsterInfo.id = MONSTER_GOBLIN_A;
    actor.monsterId = MONSTER_INVALID;
    actor.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    return actor;
}

} // namespace

UNIT_TEST(MM6V5PreEventActorReplayForTest, RequiresV3BaselineThenRestoresExactPosition) {
    const Actor original = baselineActor();
    const std::array<Actor, 1> baseline{original};
    const auto baselineBytes = mm6::encodeActiveActorSection(baseline, GameTitle::MM6);
    Actor dynamic = original;
    dynamic.pos = Vec3f(1.25f, -2.5f, 3.75f);
    const std::array<Actor, 1> action{dynamic};
    mm6::testing::ScopedV5PreEventActorReplayForTest scope(
        GameTitle::MM6,
        mm6::captureActorRuntimeFields("oute3.odm", baselineBytes, action, GameTitle::MM6),
        baselineBytes);
    std::deque<Actor> live{original};
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, false,
        "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, true,
        "oute3.odm", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(std::bit_cast<std::uint32_t>(live[0].pos.x),
              std::bit_cast<std::uint32_t>(dynamic.pos.x));
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, true,
        "oute3.odm", GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V5PreEventActorReplayForTest, RefusesChangedPublishedBaseline) {
    const Actor original = baselineActor();
    const std::array<Actor, 1> baseline{original};
    const auto baselineBytes = mm6::encodeActiveActorSection(baseline, GameTitle::MM6);
    mm6::testing::ScopedV5PreEventActorReplayForTest scope(
        GameTitle::MM6,
        mm6::captureActorRuntimeFields("oute3.odm", baselineBytes, baseline, GameTitle::MM6),
        baselineBytes);
    std::deque<Actor> live{original};
    live[0].hp++;
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventActorReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, live, &live, true,
        "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
}
