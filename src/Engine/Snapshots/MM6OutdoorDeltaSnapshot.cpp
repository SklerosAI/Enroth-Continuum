#include "MM6OutdoorDeltaSnapshot.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6MonsterDamage.h"
#include "Engine/MM6/MM6MonsterProjectile.h"
#include "Engine/MM6/MM6MonsterSpecialAttack.h"

#include "Utility/Memory/Blob.h"

namespace {

static_assert(std::endian::native == std::endian::little);
static_assert(offsetof(Actor_MM7, field_84) == 132);
static_assert(offsetof(Actor_MM7, group) == 708);
static_assert(offsetof(SpriteObject_MM7, containing_item) == 36);
static_assert(offsetof(SpriteObject_MM7, uSpellID) == 72);

void copyBytes(void *dst, const void *src, size_t offset, size_t length) {
    std::memcpy(static_cast<std::uint8_t *>(dst) + offset, static_cast<const std::uint8_t *>(src) + offset, length);
}

void write16(std::span<std::uint8_t> bytes, size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void write32(std::span<std::uint8_t> bytes, size_t offset, std::uint32_t value) {
    write16(bytes, offset, value);
    write16(bytes, offset + 2, value >> 16);
}

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
    // has different spell/resistance slots. Project only matching fields,
    // with attack kinds, missiles and special attacks in runtime terms.
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
    monster.specialAttackType = std::to_underlying(mm6::specialAttackForNativeByte(bytes[64]));
    monster.specialAttackLevel = bytes[65];
    monster.attack1Type = std::to_underlying(mm6::damageTypeForEventKind(bytes[66]));
    monster.attack1DamageDiceRolls = bytes[67];
    monster.attack1DamageDiceSides = bytes[68];
    monster.attack1DamageBonus = bytes[69];
    monster.attack1MissileType = std::to_underlying(mm6::monsterProjectileForNativeByte(bytes[70]));
    monster.attack2Chance = bytes[71];
    monster.attack2Type = std::to_underlying(mm6::damageTypeForEventKind(bytes[72]));
    monster.attack2DamageDiceRolls = bytes[73];
    monster.attack2DamageDiceSides = bytes[74];
    monster.attack2DamageBonus = bytes[75];
    monster.attack2MissileType = std::to_underlying(mm6::monsterProjectileForNativeByte(bytes[76]));
    monster.spell1UseChance = bytes[77];
    monster.spell1Id = bytes[78];
    monster.spell1SkillMastery = bytes[79]; // The level, plus 0x40 for Expert and 0x80 for Master, as the runtime joins them.
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
    // The MM6 wire carries all 140 signed cells; the 9x9 active area is
    // validated before changed-state encoding.
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

Blob encodeMM6OutdoorRuntimeDelta(const MM6OutdoorDeltaSnapshot &delta) {
    if (delta.wire.bytes.empty())
        throw std::runtime_error("MM6 outdoor writer requires a decoded DDM sidecar");
    MM6OutdoorDeltaSnapshot original = decodeMM6OutdoorDelta(
        Blob::copy(delta.wire.bytes.data(), delta.wire.bytes.size()));
    if (delta.actors.size() != original.actors.size() ||
        delta.spriteObjects.size() != original.spriteObjects.size() ||
        delta.chests.size() != original.chests.size())
        throw std::runtime_error("MM6 outdoor writer cannot change record counts");
    if (delta.respawnCount > std::numeric_limits<int>::max() ||
        delta.lastRespawnDay > std::numeric_limits<int>::max())
        throw std::runtime_error("MM6 outdoor reset counter exceeds runtime range");

    mm6::OutdoorDeltaWire wire = std::move(original.wire);
    auto reset = wire.part(mm6::OutdoorDeltaPart::ResetHeader);
    write32(reset, 0, delta.respawnCount);
    write32(reset, 4, delta.lastRespawnDay);
    auto visibility = wire.part(mm6::OutdoorDeltaPart::Visibility);
    std::memcpy(visibility.data(), delta.fullyRevealedCells.data(), 88 * 11);
    std::memcpy(visibility.data() + 88 * 11, delta.partiallyRevealedCells.data(), 88 * 11);

    auto actors = wire.part(mm6::OutdoorDeltaPart::Actors).subspan(4);
    for (size_t i = 0; i < delta.actors.size(); ++i) {
        const Actor_MM7 &current = delta.actors[i];
        Actor_MM7 permitted = original.actors[i];
        if (current.hp < 0)
            throw std::runtime_error("MM6 outdoor actor has unverified negative HP encoding");
        permitted.attributes = current.attributes;
        permitted.hp = current.hp;
        // MM6 118..165 and 168..195 align to MM7 134..181 and 184..211.
        // Keep the intervening padding words and all other fields verbatim.
        copyBytes(&permitted, &current, 134, 48);
        copyBytes(&permitted, &current, 184, 28);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw std::runtime_error("MM6 outdoor actor has unsupported changed fields");
        auto record = actors.subspan(i * 548, 548);
        write32(record, 36, current.attributes);
        write16(record, 40, current.hp);
        std::memcpy(record.data() + 118, reinterpret_cast<const std::uint8_t *>(&current) + 134, 48);
        std::memcpy(record.data() + 168, reinterpret_cast<const std::uint8_t *>(&current) + 184, 28);
    }

    auto ground = wire.part(mm6::OutdoorDeltaPart::GroundItems).subspan(4);
    for (size_t i = 0; i < delta.spriteObjects.size(); ++i) {
        const SpriteObject_MM7 &current = delta.spriteObjects[i];
        SpriteObject_MM7 permitted = original.spriteObjects[i];
        copyBytes(&permitted, &current, 0, 24);
        // The contained MM6 item has mapped fields only through byte 25.
        // Keep its final two bytes from the sidecar.
        copyBytes(&permitted, &current, 26, 36);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw std::runtime_error("MM6 outdoor ground item has unsupported changed fields");
        auto record = ground.subspan(i * 100, 100);
        std::memcpy(record.data(), &current, 24);
        std::memcpy(record.data() + 26, reinterpret_cast<const std::uint8_t *>(&current) + 26, 36);
    }

    auto chests = wire.part(mm6::OutdoorDeltaPart::Chests).subspan(4);
    for (size_t i = 0; i < delta.chests.size(); ++i) {
        const Chest_MM7 &current = delta.chests[i];
        if (!mm6::validChestRuntimeGrid(GameTitle::MM6, current))
            throw std::runtime_error("MM6 outdoor chest has invalid active grid");
        Chest_MM7 permitted = original.chests[i];
        permitted.flags = current.flags;
        permitted.inventoryMatrix = current.inventoryMatrix;
        for (size_t j = 0; j < current.items.size(); ++j)
            std::memcpy(&permitted.items[j], &current.items[j], 28);
        if (std::memcmp(&permitted, &current, sizeof(current)) != 0)
            throw std::runtime_error("MM6 outdoor chest has unsupported type or item fields");
        auto record = chests.subspan(i * 4204, 4204);
        write16(record, 2, current.flags);
        for (size_t j = 0; j < current.items.size(); ++j)
            std::memcpy(record.data() + 4 + j * 28, &current.items[j], 28);
        std::memcpy(record.data() + 3924, current.inventoryMatrix.data(), 280);
    }

    std::vector<std::uint8_t> encoded = mm6::encodeOutdoorDelta(wire);
    (void) decodeMM6OutdoorDelta(Blob::copy(encoded.data(), encoded.size()));
    return Blob::copy(encoded.data(), encoded.size());
}

enum class OutdoorActorAndSpriteRecords { Live, AsLoaded };

static Blob encodeOutdoorLiveState(const OutdoorLocation &location, OutdoorActorAndSpriteRecords records) {
    const bool withActorsAndSprites = records == OutdoorActorAndSpriteRecords::Live;
    if (pOutdoor != &location || location.mm6DeltaBytes.empty() ||
        location.ddm.respawnCount < 0 || location.ddm.lastRespawnDay < 0)
        throw std::runtime_error("MM6 outdoor runtime lacks bounded delta state");
    MM6OutdoorDeltaSnapshot current = decodeMM6OutdoorDelta(
        Blob::copy(location.mm6DeltaBytes.data(), location.mm6DeltaBytes.size()));
    if ((withActorsAndSprites && (pActors.size() != current.actors.size() || pSpriteObjects.size() != current.spriteObjects.size())) ||
        vChests.size() != current.chests.size())
        throw std::runtime_error("MM6 outdoor runtime record count changed; save encoding is not established");
    current.respawnCount = location.ddm.respawnCount;
    current.lastRespawnDay = location.ddm.lastRespawnDay;
    current.fullyRevealedCells = location.uFullyRevealedCellOnMap;
    current.partiallyRevealedCells = location.uPartiallyRevealedCellOnMap;
    for (size_t i = 0; withActorsAndSprites && i < current.actors.size(); ++i) {
        Actor_MM7 live{};
        snapshot(pActors[i], &live);
        current.actors[i].attributes = live.attributes;
        current.actors[i].hp = live.hp;
        copyBytes(&current.actors[i], &live, 134, 48);
        copyBytes(&current.actors[i], &live, 184, 28);
    }
    for (size_t i = 0; withActorsAndSprites && i < current.spriteObjects.size(); ++i) {
        SpriteObject_MM7 live{};
        snapshot(pSpriteObjects[i], &live);
        copyBytes(&current.spriteObjects[i], &live, 0, 24);
        copyBytes(&current.spriteObjects[i], &live, 26, 36);
    }
    for (size_t i = 0; i < current.chests.size(); ++i) {
        Chest_MM7 live{};
        snapshot(vChests[i], &live);
        if (!mm6::validChestRuntimeGrid(GameTitle::MM6, live))
            throw std::runtime_error("MM6 outdoor live chest has invalid active grid");
        current.chests[i].inventoryMatrix = live.inventoryMatrix;
        current.chests[i].flags = live.flags;
        for (size_t j = 0; j < live.items.size(); ++j)
            std::memcpy(&current.chests[i].items[j], &live.items[j], 24);
    }
    return encodeMM6OutdoorRuntimeDelta(current);
}

Blob encodeMM6OutdoorChangedState(const OutdoorLocation &location) {
    return encodeOutdoorLiveState(location, OutdoorActorAndSpriteRecords::Live);
}

Blob encodeMM6OutdoorParkedState(const OutdoorLocation &location) {
    return encodeOutdoorLiveState(location, OutdoorActorAndSpriteRecords::AsLoaded);
}
