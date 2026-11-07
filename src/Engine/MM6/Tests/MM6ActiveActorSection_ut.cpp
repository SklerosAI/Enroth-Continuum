#include <array>
#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6SpawnedActorSection.h"
#include "Engine/Objects/Actor.h"

namespace {

Actor actor(int slot) {
    Actor result(slot);
    result.monsterInfo.id = static_cast<MonsterId>(7);
    result.monsterId = MONSTER_INVALID;
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.initialPosition = Vec3f(10, 20, 30);
    result.pos = Vec3f(11, 20, 30);
    result.hp = 37;
    return result;
}

} // namespace

UNIT_TEST(MM6ActiveActorSection, RoundTripsFullPidListWithDistinctTag) {
    std::array<Actor, 2> actors = {actor(0), actor(1)};
    actors[1].hp = 19;
    const std::vector<std::uint8_t> bytes = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    EXPECT_EQ(bytes[0], 'A');
    EXPECT_EQ(bytes[1], 'C');
    EXPECT_EQ(bytes[2], 'T');
    EXPECT_EQ(bytes[3], 'R');
    const auto replay = mm6::decodeActiveActorSection(bytes, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 2);
    EXPECT_EQ(replay[0].id, 0);
    EXPECT_EQ(replay[1].id, 1);
    EXPECT_EQ(replay[0].monsterId, MONSTER_INVALID);
    EXPECT_EQ(replay[1].hp, 19);
    EXPECT_EQ(mm6::encodeActiveActorSection(replay, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 0, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6ActiveActorSection, RejectsWrongTitleSlotAndUnsupportedActor) {
    std::array<Actor, 1> actors = {actor(0)};
    const auto bytes = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    EXPECT_THROW((void) mm6::encodeActiveActorSection(actors, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeActiveActorSection(bytes, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeActiveActorSection(
        mm6::encodeSpawnedActorSection(actors, 0, GameTitle::MM6), GameTitle::MM6), std::exception);
    actors[0].id = 1;
    EXPECT_THROW((void) mm6::encodeActiveActorSection(actors, GameTitle::MM6), std::exception);
    actors[0] = actor(0);
    actors[0].items[0].itemId = static_cast<ItemId>(1);
    EXPECT_THROW((void) mm6::encodeActiveActorSection(actors, GameTitle::MM6), std::exception);
    auto bad = bytes;
    bad[8] = 1; // ACTR must start at PID slot zero.
    EXPECT_THROW((void) mm6::decodeActiveActorSection(bad, GameTitle::MM6), std::exception);
    bad = bytes;
    bad.pop_back();
    EXPECT_THROW((void) mm6::decodeActiveActorSection(bad, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6ActiveActorSection, CanonicalEmptyList) {
    const std::array<Actor, 0> none{};
    const auto bytes = mm6::encodeActiveActorSection(none, GameTitle::MM6);
    EXPECT_EQ(bytes.size(), 16);
    EXPECT_TRUE(mm6::decodeActiveActorSection(bytes, GameTitle::MM6).empty());
}
