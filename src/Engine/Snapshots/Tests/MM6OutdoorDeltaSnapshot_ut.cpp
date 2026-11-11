#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "Utility/Memory/Blob.h"

namespace {

void put32(std::string *data, size_t offset, std::uint32_t value) {
    for (size_t i = 0; i < 4; ++i)
        (*data)[offset + i] = static_cast<char>(value >> (8 * i));
}

Blob fixture() {
    std::string data(8 + 2 * 88 * 11 + 4 + 548 + 4 + 100 + 4 + 4204 + 200 + 56, '\0');
    size_t offset = 0;
    put32(&data, offset, 3);
    put32(&data, offset + 4, 12);
    offset += 8;
    data[offset] = static_cast<char>(0x80);
    data[offset + 88 * 11] = static_cast<char>(0x40);
    offset += 2 * 88 * 11;
    put32(&data, offset, 1);
    offset += 4;
    data[offset + 52] = static_cast<char>(173);
    data[offset + 53] = static_cast<char>(42);
    put32(&data, offset + 92, 1000);
    offset += 548;
    put32(&data, offset, 1);
    offset += 4;
    put32(&data, offset + 4, 1234);
    put32(&data, offset + 36, 20);
    data[offset + 36 + 26] = static_cast<char>(0xB4);
    data[offset + 36 + 27] = static_cast<char>(0xB5);
    offset += 100;
    put32(&data, offset, 1);
    offset += 4;
    data[offset] = 7;
    put32(&data, offset + 4, 21);
    data[offset + 4 + 26] = static_cast<char>(0xC4);
    data[offset + 4 + 27] = static_cast<char>(0xC5);
    offset += 4204;
    data[offset] = static_cast<char>(0xA5);
    data[offset + 200] = static_cast<char>(0x5A);
    return Blob::fromString(std::move(data));
}

} // namespace

UNIT_TEST(MM6OutdoorDeltaSnapshot, ProjectsKnownFieldsAndRetainsOpaqueBytes) {
    MM6OutdoorDeltaSnapshot delta = decodeMM6OutdoorDelta(fixture());
    EXPECT_EQ(delta.respawnCount, 3);
    EXPECT_EQ(delta.lastRespawnDay, 12);
    EXPECT_EQ(delta.fullyRevealedCells[0][0], 0x80);
    EXPECT_EQ(delta.partiallyRevealedCells[0][0], 0x40);
    ASSERT_EQ(delta.actors.size(), 1);
    EXPECT_EQ(delta.actors[0].monsterInfo.id, 173);
    EXPECT_EQ(delta.actors[0].monsterInfo.level, 42);
    EXPECT_EQ(delta.actors[0].monsterInfo.hp, 1000);
    ASSERT_EQ(delta.spriteObjects.size(), 1);
    EXPECT_EQ(delta.spriteObjects[0].position.x, 1234);
    EXPECT_EQ(delta.spriteObjects[0].containing_item.itemId, 20);
    ASSERT_EQ(delta.chests.size(), 1);
    EXPECT_EQ(delta.chests[0].chestTypeId, 7);
    EXPECT_EQ(delta.chests[0].items[0].itemId, 21);
    EXPECT_EQ(delta.wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[0], 0xA5);
    EXPECT_EQ(delta.wire.part(mm6::OutdoorDeltaPart::Tail)[0], 0x5A);
    EXPECT_EQ(mm6::encodeOutdoorDelta(delta.wire), delta.wire.bytes);
}

UNIT_TEST(MM6OutdoorDeltaSnapshot, RejectsBadChestTypeAndTruncation) {
    Blob good = fixture();
    MM6OutdoorDeltaSnapshot delta = decodeMM6OutdoorDelta(good);
    const size_t chestOffset = delta.wire.parts[static_cast<size_t>(mm6::OutdoorDeltaPart::Chests)].offset + 4;
    std::string bad(static_cast<const char *>(good.data()), good.size());
    bad[chestOffset] = 8;
    EXPECT_THROW(decodeMM6OutdoorDelta(Blob::fromString(std::move(bad))), std::runtime_error);
    std::string shortData(static_cast<const char *>(good.data()), good.size() - 1);
    EXPECT_THROW(decodeMM6OutdoorDelta(Blob::fromString(std::move(shortData))), std::runtime_error);
    EXPECT_THROW(projectMM6ActorRecord({}), std::runtime_error);
    EXPECT_THROW(projectMM6GroundItemRecord({}), std::runtime_error);
    EXPECT_THROW(projectMM6ChestRecord({}), std::runtime_error);
}

UNIT_TEST(MM6OutdoorDeltaSnapshot, EncodesBoundedChangedStateWithoutLosingOpaqueBytes) {
    MM6OutdoorDeltaSnapshot delta = decodeMM6OutdoorDelta(fixture());
    delta.respawnCount = 9;
    delta.lastRespawnDay = 123;
    delta.fullyRevealedCells[1][2] = 0x81;
    delta.partiallyRevealedCells[3][4] = 0x42;
    delta.actors[0].attributes = 0x12345678;
    delta.actors[0].hp = 37;
    delta.actors[0].pos.x = 555;
    delta.spriteObjects[0].position.y = 678;
    delta.spriteObjects[0].containing_item.itemId = 22;
    delta.chests[0].flags = 0x1236;
    delta.chests[0].items[0].itemId = 23;
    delta.chests[0].inventoryMatrix[0] = 1;
    delta.chests[0].inventoryMatrix[139] = -81;
    delta.wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[70] = 0x91;

    Blob encoded = encodeMM6OutdoorRuntimeDelta(delta);
    MM6OutdoorDeltaSnapshot changed = decodeMM6OutdoorDelta(encoded);
    EXPECT_EQ(changed.respawnCount, 9);
    EXPECT_EQ(changed.lastRespawnDay, 123);
    EXPECT_EQ(changed.fullyRevealedCells[1][2], 0x81);
    EXPECT_EQ(changed.partiallyRevealedCells[3][4], 0x42);
    EXPECT_EQ(changed.actors[0].attributes, 0x12345678);
    EXPECT_EQ(changed.actors[0].hp, 37);
    EXPECT_EQ(changed.actors[0].pos.x, 555);
    EXPECT_EQ(changed.spriteObjects[0].position.y, 678);
    EXPECT_EQ(changed.spriteObjects[0].containing_item.itemId, 22);
    EXPECT_EQ(changed.spriteObjects[0].containing_item.lichJarCharacterIndex, 0xB4);
    EXPECT_EQ(static_cast<unsigned char>(changed.spriteObjects[0].containing_item._pad), 0xB5);
    EXPECT_EQ(changed.chests[0].flags, 0x1236);
    EXPECT_EQ(changed.chests[0].items[0].itemId, 23);
    EXPECT_EQ(changed.chests[0].inventoryMatrix[0], 1);
    EXPECT_EQ(changed.chests[0].inventoryMatrix[139], -81);
    EXPECT_EQ(changed.chests[0].items[0].lichJarCharacterIndex, 0xC4);
    EXPECT_EQ(static_cast<unsigned char>(changed.chests[0].items[0]._pad), 0xC5);
    EXPECT_EQ(changed.wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[0], 0xA5);
    EXPECT_EQ(changed.wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[70], 0x91);
    EXPECT_EQ(changed.wire.part(mm6::OutdoorDeltaPart::Tail)[0], 0x5A);
}

UNIT_TEST(MM6OutdoorDeltaSnapshot, RejectsUnsupportedChangedState) {
    MM6OutdoorDeltaSnapshot delta = decodeMM6OutdoorDelta(fixture());
    delta.actors[0].monsterInfo.level++;
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
    delta = decodeMM6OutdoorDelta(fixture());
    delta.actors[0].hp = -1;
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
    delta = decodeMM6OutdoorDelta(fixture());
    delta.chests[0].inventoryMatrix[0] = 82;
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
    delta = decodeMM6OutdoorDelta(fixture());
    delta.actors.pop_back();
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
    delta = decodeMM6OutdoorDelta(fixture());
    delta.spriteObjects[0].uSpellID++;
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
    delta = decodeMM6OutdoorDelta(fixture());
    delta.spriteObjects[0].containing_item._pad++;
    EXPECT_THROW(encodeMM6OutdoorRuntimeDelta(delta), std::runtime_error);
}

UNIT_TEST(MM6OutdoorDeltaSnapshot, RetainedFifteenDeltas) {
    const char *directory = std::getenv("ENROTH_MM6_OUTDOOR_DECODED_DIR");
    if (!directory)
        GTEST_SKIP() << "Set ENROTH_MM6_OUTDOOR_DECODED_DIR to ignored decoded fixtures";

    size_t count = 0;
    size_t actors = 0;
    size_t items = 0;
    size_t chests = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".ddm")
            continue;
        std::ifstream stream(entry.path(), std::ios::binary);
        ASSERT_TRUE(stream.good());
        std::string data(std::istreambuf_iterator<char>{stream}, {});
        MM6OutdoorDeltaSnapshot delta = decodeMM6OutdoorDelta(Blob::fromString(std::move(data)));
        EXPECT_EQ(mm6::encodeOutdoorDelta(delta.wire), delta.wire.bytes);
        Blob encoded = encodeMM6OutdoorRuntimeDelta(delta);
        EXPECT_EQ(std::string(static_cast<const char *>(encoded.data()), encoded.size()),
                  std::string(reinterpret_cast<const char *>(delta.wire.bytes.data()), delta.wire.bytes.size()));
        delta.wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[70] = 0x91;
        Blob withMapVar = encodeMM6OutdoorRuntimeDelta(delta);
        EXPECT_EQ(decodeMM6OutdoorDelta(withMapVar).wire.part(mm6::OutdoorDeltaPart::PersistentBytes)[70], 0x91);
        actors += delta.actors.size();
        items += delta.spriteObjects.size();
        chests += delta.chests.size();
        ++count;
    }
    EXPECT_EQ(count, 15);
    EXPECT_GT(actors, 0);
    EXPECT_GT(items, 0);
    EXPECT_EQ(chests, 300);
}

UNIT_TEST(MM6OutdoorDeltaSnapshot, ProjectsActorAttacksInRuntimeTerms) {
    // A placed MM6 actor used to keep its raw attack bytes, so the reactor's Energy missile refused to launch.
    std::array<std::uint8_t, 548> record{};
    record[64] = 23; // Afraid.
    record[66] = 6; // Energy.
    record[70] = 7; // Energy missile.
    record[72] = 2; // Fire.
    record[76] = 9; // Rock.
    record[79] = 0x80 | 14; // Master, level 14.
    const Actor_MM7 actor = projectMM6ActorRecord(record);
    EXPECT_EQ(actor.monsterInfo.specialAttackType, std::to_underlying(SPECIAL_ATTACK_FEAR));
    EXPECT_EQ(actor.monsterInfo.attack1Type, std::to_underlying(DAMAGE_ENERGY));
    EXPECT_EQ(actor.monsterInfo.attack1MissileType, std::to_underlying(MONSTER_PROJECTILE_MM6_ENERGY));
    EXPECT_EQ(actor.monsterInfo.attack2Type, std::to_underlying(DAMAGE_FIRE));
    EXPECT_EQ(actor.monsterInfo.attack2MissileType, std::to_underlying(MONSTER_PROJECTILE_MM6_ROCK));
    EXPECT_EQ(CombinedSkillValue::fromJoined(actor.monsterInfo.spell1SkillMastery), CombinedSkillValue(14, MASTERY_MASTER));
}
