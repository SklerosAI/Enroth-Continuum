#include <array>
#include <bit>
#include <cstdint>
#include <exception>
#include <limits>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6V5ActorRuntimeFields.h"

namespace {

Actor actor() {
    Actor result(0);
    result.monsterInfo.id = static_cast<MonsterId>(7);
    result.monsterId = MONSTER_INVALID;
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.initialPosition = Vec3f(10, 20, 30);
    result.pos = Vec3f(11, 20, 30);
    result.hp = 37;
    return result;
}

std::vector<std::uint8_t> baseline(const Actor &source) {
    return mm6::encodeActiveActorSection(std::array<Actor, 1>{source}, GameTitle::MM6);
}

} // namespace

UNIT_TEST(MM6V5ActorRuntimeFields, ExactFractionalRuntimeRoundTrip) {
    const Actor source = actor();
    const auto anchor = baseline(source);
    Actor moved = source;
    moved.pos = Vec3f(11.25f, 20.5f, 30.75f);
    moved.velocity = Vec3f(0.125f, -0.5f, 1.25f);
    moved.guardingPosition = Vec3f(10.5f, 20.25f, 30);
    moved.scheduledJobs[0].vPos = Vec3f(2.5f, 3.25f, 4.125f);
    moved.currentActionTime = Duration::fromTicks(100000);
    moved.donebloodsplat = true;
    const auto fields = mm6::captureActorRuntimeFields(
        "o0.odm", anchor, std::array<Actor, 1>{moved}, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeActorRuntimeSection(fields, GameTitle::MM6));
    const auto replay = mm6::decodeActorRuntimeSection(container, GameTitle::MM6);
    ASSERT_EQ(replay.actors.size(), 1);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(replay.actors[0].pos.x),
              std::bit_cast<std::uint32_t>(moved.pos.x));
    EXPECT_EQ(replay.actors[0].velocity, moved.velocity);
    EXPECT_EQ(replay.actors[0].scheduledJobs[0].vPos, moved.scheduledJobs[0].vPos);
    EXPECT_EQ(replay.actors[0].currentActionTime, moved.currentActionTime);
    EXPECT_TRUE(replay.actors[0].donebloodsplat);
    EXPECT_EQ(mm6::encodeActorRuntimeSection(replay, GameTitle::MM6),
              container.sections[0]);
}

UNIT_TEST(MM6V5ActorRuntimeFields, RejectsChangedAnchorAndUnsupportedOwners) {
    const Actor source = actor();
    const auto anchor = baseline(source);
    Actor changed = source;
    changed.initialPosition.x = 10.5f;
    EXPECT_THROW((void) mm6::captureActorRuntimeFields(
        "o0.odm", anchor, std::array<Actor, 1>{changed}, GameTitle::MM6),
        std::exception);
    changed = source;
    changed.items[0].itemId = static_cast<ItemId>(1);
    EXPECT_THROW((void) mm6::captureActorRuntimeFields(
        "o0.odm", anchor, std::array<Actor, 1>{changed}, GameTitle::MM6),
        std::exception);
    changed = source;
    changed.pos.x = std::numeric_limits<float>::infinity();
    EXPECT_THROW((void) mm6::captureActorRuntimeFields(
        "o0.odm", anchor, std::array<Actor, 1>{changed}, GameTitle::MM6),
        std::exception);
    changed = source;
    changed.velocity.y = 40000;
    EXPECT_THROW((void) mm6::captureActorRuntimeFields(
        "o0.odm", anchor, std::array<Actor, 1>{changed}, GameTitle::MM6),
        std::exception);
}
