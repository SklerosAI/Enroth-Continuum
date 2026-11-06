#include "MM6OutdoorDeltaSnapshot.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>

#include "Utility/Memory/Blob.h"

namespace {

static_assert(std::endian::native == std::endian::little);
static_assert(offsetof(Actor_MM7, field_84) == 132);
static_assert(offsetof(Actor_MM7, group) == 708);
static_assert(offsetof(SpriteObject_MM7, containing_item) == 36);
static_assert(offsetof(SpriteObject_MM7, uSpellID) == 72);

std::uint16_t read16(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint16_t(bytes[offset]) | (std::uint16_t(bytes[offset + 1]) << 8);
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint32_t(read16(bytes, offset)) | (std::uint32_t(read16(bytes, offset + 2)) << 16);
}

Actor_MM7 actorForRuntime(std::span<const std::uint8_t> bytes) {
    Actor_MM7 result{};
    std::memcpy(result.name.data(), bytes.data(), result.name.size());
    result.npcId = read16(bytes, 32);
    result.attributes = read32(bytes, 36);
    result.hp = read16(bytes, 40);

    // MM6's embedded monster descriptor is 72 bytes; MM7's is 88 bytes and
    // has different spell/resistance slots. Project only matching fields.
    auto &monster = result.monsterInfo;
    monster.id = bytes[52];
    monster.level = bytes[53];
    monster.treasureDropChance = bytes[54];
    monster.goldDiceRolls = bytes[55];
    monster.goldDiceSides = bytes[56];
    monster.treasureLevel = bytes[57];
    monster.treasureType = bytes[58];
    monster.flying = bytes[59];
    monster.movementType = bytes[60];
    monster.aiType = bytes[61];
    monster.hostilityType = bytes[62];
    monster.specialAttackType = bytes[64];
    monster.specialAttackLevel = bytes[65];
    monster.attack1Type = bytes[66];
    monster.attack1DamageDiceRolls = bytes[67];
    monster.attack1DamageDiceSides = bytes[68];
    monster.attack1DamageBonus = bytes[69];
    monster.attack1MissileType = bytes[70];
    monster.attack2Chance = bytes[71];
    monster.attack2Type = bytes[72];
    monster.attack2DamageDiceRolls = bytes[73];
    monster.attack2DamageDiceSides = bytes[74];
    monster.attack2DamageBonus = bytes[75];
    monster.attack2MissileType = bytes[76];
    monster.spell1UseChance = bytes[77];
    monster.spell1Id = bytes[78];
    monster.resFire = bytes[80];
    monster.resAir = bytes[81];
    monster.resWater = bytes[82];
    monster.resBody = bytes[83];
    monster.resPhysical = bytes[84];
    monster.specialAbilityType = bytes[86];
    monster.specialAbilityDamageDiceRolls = bytes[87];
    monster.specialAbilityDamageDiceSides = bytes[88];
    monster.specialAbilityDamageDiceBonus = bytes[89];
    monster.hp = read32(bytes, 92);
    monster.ac = read32(bytes, 96);
    monster.exp = read32(bytes, 100);
    monster.baseSpeed = read32(bytes, 104);
    monster.recoveryTime = read32(bytes, 108);

    // From field_84 through sound IDs, and from group through the reserved
    // tail, MM6 and MM7 have the same field widths and order.
    std::memcpy(reinterpret_cast<std::uint8_t *>(&result) + offsetof(Actor_MM7, field_84), bytes.data() + 116, 80);
    std::memcpy(reinterpret_cast<std::uint8_t *>(&result) + offsetof(Actor_MM7, group), bytes.data() + 420, 128);
    return result;
}

SpriteObject_MM7 spriteForRuntime(std::span<const std::uint8_t> bytes) {
    SpriteObject_MM7 result{};
    std::memcpy(&result, bytes.data(), 36);
    std::memcpy(&result.containing_item, bytes.data() + 36, 28);
    std::memcpy(reinterpret_cast<std::uint8_t *>(&result) + offsetof(SpriteObject_MM7, uSpellID), bytes.data() + 64, 36);
    return result;
}

Chest_MM7 chestForRuntime(std::span<const std::uint8_t> bytes) {
    Chest_MM7 result{};
    result.chestTypeId = read16(bytes, 0);
    if (result.chestTypeId >= 8)
        throw std::runtime_error("MM6 outdoor chest type exceeds runtime table");
    result.flags = read16(bytes, 2);
    for (size_t i = 0; i < result.items.size(); ++i)
        std::memcpy(&result.items[i], bytes.data() + 4 + i * 28, 28);
    // The shipped grid is zero. Copy all 280 bytes, but keep the original in
    // the wire sidecar because MM6's 81-vs-140-cell interpretation is open.
    std::memcpy(result.inventoryMatrix.data(), bytes.data() + 3924, 280);
    return result;
}

} // namespace

Actor_MM7 projectMM6ActorRecord(std::span<const std::uint8_t> record) {
    if (record.size() != 548)
        throw std::runtime_error("MM6 actor record must be 548 bytes");
    return actorForRuntime(record);
}

SpriteObject_MM7 projectMM6GroundItemRecord(std::span<const std::uint8_t> record) {
    if (record.size() != 100)
        throw std::runtime_error("MM6 ground item record must be 100 bytes");
    return spriteForRuntime(record);
}

Chest_MM7 projectMM6ChestRecord(std::span<const std::uint8_t> record) {
    if (record.size() != 4204)
        throw std::runtime_error("MM6 chest record must be 4204 bytes");
    return chestForRuntime(record);
}

MM6OutdoorDeltaSnapshot decodeMM6OutdoorDelta(const Blob &source) {
    MM6OutdoorDeltaSnapshot result;
    result.wire = mm6::decodeOutdoorDelta({static_cast<const std::uint8_t *>(source.data()), source.size()});

    const auto reset = result.wire.part(mm6::OutdoorDeltaPart::ResetHeader);
    result.respawnCount = read32(reset, 0);
    result.lastRespawnDay = read32(reset, 4);
    if (result.respawnCount > std::numeric_limits<int>::max() ||
        result.lastRespawnDay > std::numeric_limits<int>::max())
        throw std::runtime_error("MM6 outdoor reset counter exceeds runtime range");

    const auto visibility = result.wire.part(mm6::OutdoorDeltaPart::Visibility);
    std::memcpy(result.fullyRevealedCells.data(), visibility.data(), 88 * 11);
    std::memcpy(result.partiallyRevealedCells.data(), visibility.data() + 88 * 11, 88 * 11);

    const auto actors = result.wire.part(mm6::OutdoorDeltaPart::Actors).subspan(4);
    result.actors.reserve(result.wire.actorCount);
    for (size_t i = 0; i < result.wire.actorCount; ++i)
        result.actors.push_back(projectMM6ActorRecord(actors.subspan(i * 548, 548)));

    const auto sprites = result.wire.part(mm6::OutdoorDeltaPart::GroundItems).subspan(4);
    result.spriteObjects.reserve(result.wire.groundItemCount);
    for (size_t i = 0; i < result.wire.groundItemCount; ++i)
        result.spriteObjects.push_back(projectMM6GroundItemRecord(sprites.subspan(i * 100, 100)));

    const auto chests = result.wire.part(mm6::OutdoorDeltaPart::Chests).subspan(4);
    result.chests.reserve(result.wire.chestCount);
    for (size_t i = 0; i < result.wire.chestCount; ++i)
        result.chests.push_back(projectMM6ChestRecord(chests.subspan(i * 4204, 4204)));
    return result;
}
