#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpawnedActorSection.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Snapshots/EntitySnapshots.h"

namespace {

Actor spawned(int slot) {
    Actor result(slot);
    result.monsterInfo.id = static_cast<MonsterId>(7);
    result.monsterId = static_cast<MonsterId>(12);
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    result.hp = 37;
    result.initialPosition = Vec3f(10, 20, 30);
    result.pos = Vec3f(11, 20, 30);
    result.group = 42;
    result.massDistortionTime = Duration::fromTicks(123);
    return result;
}

} // namespace

UNIT_TEST(MM6SpawnedActorSection, RoundTripsExactSlotsAndMM6Resistance) {
    std::array<Actor, 2> actors = {spawned(38), spawned(39)};
    actors[1].npcId = 4;
    actors[1].donebloodsplat = true;
    const std::vector<std::uint8_t> bytes = mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6);
    const std::vector<Actor> replay = mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 2);
    EXPECT_EQ(replay[0].id, 38);
    EXPECT_EQ(replay[1].id, 39);
    EXPECT_EQ(replay[0].hp, 37);
    EXPECT_EQ(replay[0].initialPosition, Vec3f(10, 20, 30));
    EXPECT_EQ(replay[0].pos, Vec3f(11, 20, 30));
    EXPECT_EQ(replay[0].group, 42);
    EXPECT_EQ(replay[1].npcId, 4);
    EXPECT_TRUE(replay[1].donebloodsplat);
    EXPECT_EQ(replay[0].massDistortionTime, Duration::fromTicks(123));
    ASSERT_TRUE(replay[0].monsterInfo.mm6Resistances);
    EXPECT_EQ(replay[0].monsterInfo.mm6Resistances->magic, 6);
    EXPECT_EQ(mm6::encodeSpawnedActorSection(replay, 38, GameTitle::MM6), bytes);
}

UNIT_TEST(MM6SpawnedActorSection, RefusesUnsupportedLiveState) {
    std::array<Actor, 1> actors = {spawned(38)};
    actors[0].items[0].itemId = static_cast<ItemId>(1);
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
    actors[0] = spawned(38);
    actors[0].buffs[ACTOR_BUFF_BLESS].power = 1;
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
    actors[0] = spawned(38);
    actors[0].pos.x = 11.5f;
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
    actors[0] = spawned(38);
    actors[0].monsterInfo.mm6Resistances.reset();
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
    actors[0] = spawned(37);
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6SpawnedActorSection, RefusesBadFramingAndTitle) {
    std::array<Actor, 1> actors = {spawned(38)};
    std::vector<std::uint8_t> bytes = mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6);
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 39, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM7), std::exception);
    bytes.pop_back();
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6), std::exception);
    bytes = mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6);
    bytes[4] = 2;
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6), std::exception);
    bytes = mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6);
    bytes[16 + offsetof(Actor_MM7, items)] = 1;
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6SpawnedActorSection, CanonicalEmptyRowsRetainBaselineSlot) {
    const std::array<Actor, 0> none{};
    const std::vector<std::uint8_t> bytes = mm6::encodeSpawnedActorSection(none, 38, GameTitle::MM6);
    EXPECT_EQ(bytes.size(), 16);
    EXPECT_TRUE(mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6).empty());
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(bytes, 39, GameTitle::MM6), std::exception);
    std::vector<std::uint8_t> noncanonical = bytes;
    noncanonical.push_back(0);
    EXPECT_THROW((void) mm6::decodeSpawnedActorSection(noncanonical, 38, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6SpawnedActorSection, PreservesExplicitZeroDescriptorButRejectsOtherInvalidIds) {
    std::array<Actor, 1> actors = {spawned(38)};
    actors[0].monsterId = MONSTER_INVALID;
    const auto bytes = mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6);
    const auto replay = mm6::decodeSpawnedActorSection(bytes, 38, GameTitle::MM6);
    ASSERT_EQ(replay.size(), 1);
    EXPECT_EQ(replay[0].monsterId, MONSTER_INVALID);
    EXPECT_EQ(replay[0].monsterInfo.id, static_cast<MonsterId>(7));
    EXPECT_EQ(mm6::encodeSpawnedActorSection(replay, 38, GameTitle::MM6), bytes);
    actors[0].monsterId = static_cast<MonsterId>(300);
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
    actors[0] = spawned(38);
    actors[0].monsterInfo.id = MONSTER_INVALID;
    EXPECT_THROW((void) mm6::encodeSpawnedActorSection(actors, 38, GameTitle::MM6), std::exception);
}
