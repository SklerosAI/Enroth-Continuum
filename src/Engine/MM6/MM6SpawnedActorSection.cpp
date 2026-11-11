#include "MM6SpawnedActorSection.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Objects/Actor.h"
#include "Engine/Snapshots/EntitySnapshots.h"

#include "Library/Snapshots/CommonSnapshots.h"

namespace mm6 {
namespace {

static_assert(std::endian::native == std::endian::little);

constexpr size_t HEADER_BYTES = 16;
constexpr size_t EXTRA_BYTES = 16;
constexpr size_t RECORD_BYTES = sizeof(Actor_MM7) + EXTRA_BYTES;
constexpr size_t MAX_ACTOR_SLOTS = 100000;
constexpr size_t MAX_SECTION_BYTES = 64 * 1024 * 1024;

void write32(std::span<std::uint8_t> bytes, size_t offset, std::uint32_t value) {
    for (size_t i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}

void write64(std::span<std::uint8_t> bytes, size_t offset, std::uint64_t value) {
    for (size_t i = 0; i < 8; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}

std::uint64_t read64(std::span<const std::uint8_t> bytes, size_t offset) {
    return std::uint64_t(read32(bytes, offset)) |
           (std::uint64_t(read32(bytes, offset + 4)) << 32);
}

bool emptyItem(const Item &item) {
    const Item empty;
    return item.itemId == empty.itemId && item.potionPower == empty.potionPower &&
           item.goldAmount == empty.goldAmount &&
           item.standardEnchantment == empty.standardEnchantment &&
           item.standardEnchantmentStrength == empty.standardEnchantmentStrength &&
           item.specialEnchantment == empty.specialEnchantment &&
           item.numCharges == empty.numCharges && item.maxCharges == empty.maxCharges &&
           item.flags == empty.flags &&
           item.lichJarCharacterIndex == empty.lichJarCharacterIndex &&
           item.enchantmentExpirationTime == empty.enchantmentExpirationTime &&
           !item.mm6RawRecord;
}

bool emptyBuff(const SpellBuff &buff) {
    const SpellBuff empty;
    return buff.expireTime == empty.expireTime && buff.power == empty.power &&
           buff.skillMastery == empty.skillMastery && buff.overlayId == empty.overlayId &&
           buff.caster == empty.caster && buff.isGM == empty.isGM;
}

void requireSupportedActor(const Actor &actor, size_t slot) {
    if (actor.id != slot || !actor.monsterInfo.mm6Resistances ||
        actor.monsterInfo.id < MONSTER_FIRST || actor.monsterInfo.id > MONSTER_LAST ||
        (actor.monsterId != MONSTER_INVALID &&
         (actor.monsterId < MONSTER_FIRST || actor.monsterId > MONSTER_LAST)))
        throw std::runtime_error("MM6 spawned actor has invalid slot or monster identity");
    if (!std::all_of(actor.items.begin(), actor.items.end(), emptyItem) ||
        !std::all_of(actor.buffs.begin(), actor.buffs.end(), emptyBuff))
        throw std::runtime_error("MM6 spawned actor item or buff state lacks EC6 representation");
}

void requirePristineWireItemsAndBuffs(const Actor_MM7 &wire) {
    SpellBuff_MM7 emptyBuffWire{};
    snapshot(SpellBuff{}, &emptyBuffWire);
    for (const SpellBuff_MM7 &buff : wire.buffs)
        if (std::memcmp(&buff, &emptyBuffWire, sizeof(buff)) != 0)
            throw std::runtime_error("MM6 spawned actor wire has unsupported buff state");
    Item_MM7 emptyItemWire{};
    snapshot(Item{}, &emptyItemWire, tags::context(ITEM_SLOT_INVALID));
    for (const Item_MM7 &item : wire.items)
        if (std::memcmp(&item, &emptyItemWire, sizeof(item)) != 0)
            throw std::runtime_error("MM6 spawned actor wire has unsupported item state");
}

// Same current-runtime fields as the existing Actor_MM7 snapshot, but without
// GetDisplayName's global NPC/table lookup. The name is derived on use and
// ignored by reconstruct; MM6 resistance and runtime-only fields live below.
Actor_MM7 snapshotWithoutName(const Actor &src) {
    Actor_MM7 result{};
    Actor_MM7 *dst = &result;
    dst->npcId = src.npcId;
    dst->attributes = std::to_underlying(src.attributes);
    dst->hp = src.hp;
    dst->monsterInfo.level = src.monsterInfo.level;
    dst->monsterInfo.treasureDropChance = src.monsterInfo.treasureDropChance;
    dst->monsterInfo.goldDiceRolls = src.monsterInfo.goldDiceRolls;
    dst->monsterInfo.goldDiceSides = src.monsterInfo.goldDiceSides;
    dst->monsterInfo.treasureLevel = std::to_underlying(src.monsterInfo.treasureLevel);
    dst->monsterInfo.treasureType = std::to_underlying(src.monsterInfo.treasureType);
    dst->monsterInfo.flying = src.monsterInfo.flying;
    dst->monsterInfo.movementType = std::to_underlying(src.monsterInfo.movementType);
    dst->monsterInfo.aiType = std::to_underlying(src.monsterInfo.aiType);
    dst->monsterInfo.hostilityType = std::to_underlying(src.monsterInfo.hostilityType);
    dst->monsterInfo.specialAttackType = std::to_underlying(src.monsterInfo.specialAttackType);
    dst->monsterInfo.specialAttackLevel = src.monsterInfo.specialAttackLevel;
    dst->monsterInfo.attack1Type = std::to_underlying(src.monsterInfo.attack1Type);
    dst->monsterInfo.attack1DamageDiceRolls = src.monsterInfo.attack1DamageDiceRolls;
    dst->monsterInfo.attack1DamageDiceSides = src.monsterInfo.attack1DamageDiceSides;
    dst->monsterInfo.attack1DamageBonus = src.monsterInfo.attack1DamageBonus;
    dst->monsterInfo.attack1MissileType = std::to_underlying(src.monsterInfo.attack1MissileType);
    dst->monsterInfo.attack2Chance = src.monsterInfo.attack2Chance;
    dst->monsterInfo.attack2Type = std::to_underlying(src.monsterInfo.attack2Type);
    dst->monsterInfo.attack2DamageDiceRolls = src.monsterInfo.attack2DamageDiceRolls;
    dst->monsterInfo.attack2DamageDiceSides = src.monsterInfo.attack2DamageDiceSides;
    dst->monsterInfo.attack2DamageBonus = src.monsterInfo.attack2DamageBonus;
    dst->monsterInfo.attack2MissileType = std::to_underlying(src.monsterInfo.attack2MissileType);
    dst->monsterInfo.spell1UseChance = src.monsterInfo.spell1UseChance;
    dst->monsterInfo.spell1Id = std::to_underlying(src.monsterInfo.spell1Id);
    dst->monsterInfo.spell2UseChance = src.monsterInfo.spell2UseChance;
    dst->monsterInfo.spell2Id = std::to_underlying(src.monsterInfo.spell2Id);
    dst->monsterInfo.resFire = src.monsterInfo.resFire;
    dst->monsterInfo.resAir = src.monsterInfo.resAir;
    dst->monsterInfo.resWater = src.monsterInfo.resWater;
    dst->monsterInfo.resEarth = src.monsterInfo.resEarth;
    dst->monsterInfo.resMind = src.monsterInfo.resMind;
    dst->monsterInfo.resSpirit = src.monsterInfo.resSpirit;
    dst->monsterInfo.resBody = src.monsterInfo.resBody;
    dst->monsterInfo.resLight = src.monsterInfo.resLight;
    dst->monsterInfo.resDark = src.monsterInfo.resDark;
    dst->monsterInfo.resPhysical = src.monsterInfo.resPhysical;
    dst->monsterInfo.specialAbilityType = std::to_underlying(src.monsterInfo.specialAbilityType);
    dst->monsterInfo.specialAbilityDamageDiceRolls = src.monsterInfo.specialAbilityDamageDiceRolls;
    dst->monsterInfo.specialAbilityDamageDiceSides = src.monsterInfo.specialAbilityDamageDiceSides;
    dst->monsterInfo.specialAbilityDamageDiceBonus = src.monsterInfo.specialAbilityDamageDiceBonus;
    dst->monsterInfo.numCharactersAttackedPerSpecialAbility = src.monsterInfo.numCharactersAttackedPerSpecialAbility;
    dst->monsterInfo.id = std::to_underlying(src.monsterInfo.id);
    dst->monsterInfo.bloodSplatOnDeath = src.monsterInfo.bloodSplatOnDeath;
    dst->monsterInfo.spell1SkillMastery = src.monsterInfo.spell1SkillMastery.joined();
    dst->monsterInfo.spell2SkillMastery = src.monsterInfo.spell2SkillMastery.joined();
    dst->monsterInfo.field_3C_some_special_attack = src.monsterInfo.field_3C_some_special_attack;
    dst->monsterInfo.field_3E = src.monsterInfo.field_3E;
    dst->monsterInfo.hp = src.monsterInfo.hp;
    dst->monsterInfo.ac = src.monsterInfo.ac;
    dst->monsterInfo.exp = src.monsterInfo.exp;
    dst->monsterInfo.baseSpeed = src.monsterInfo.baseSpeed;
    dst->monsterInfo.recoveryTime = src.monsterInfo.recoveryTime.ticks();
    dst->monsterInfo.attackPreferences = std::to_underlying(src.monsterInfo.attackPreferences);
    dst->monsterId = std::to_underlying(src.monsterId);
    dst->radius = src.radius;
    dst->height = src.height;
    dst->moveSpeed = src.moveSpeed;
    snapshot(src.pos, &dst->pos);
    snapshot(src.velocity, &dst->velocity);
    dst->yawAngle = src.yawAngle;
    dst->pitchAngle = src.pitchAngle;
    dst->sectorId = src.sectorId;
    dst->currentActionLength = src.currentActionLength.ticks();
    snapshot(src.initialPosition, &dst->initialPosition);
    snapshot(src.guardingPosition, &dst->guardingPosition);
    dst->tetherDistance = src.tetherDistance;
    dst->aiState = std::to_underlying(src.aiState);
    dst->currentActionAnimation = std::to_underlying(src.currentActionAnimation);
    dst->carriedItemId = std::to_underlying(src.carriedItemId);
    dst->currentActionTime = src.currentActionTime.ticks();
    snapshot(src.spriteIds, &dst->spriteIds);
    snapshot(src.soundSampleIds, &dst->soundSampleIds, tags::cast<SoundId, uint16_t>);
    snapshot(src.buffs, &dst->buffs);
    snapshot(src.items, &dst->items, tags::context(ITEM_SLOT_INVALID));
    dst->group = src.group;
    dst->hostilityGroup = src.hostilityGroup == MONSTER_TYPE_INVALID ? 9999 : std::to_underlying(src.hostilityGroup);
    snapshot(src.scheduledJobs, &dst->scheduledJobs);
    dst->summonerId = src.summonerId.packed();
    dst->lastCharacterIdToHit = src.lastCharacterIdToHit.packed();
    dst->uniqueNameIndex = src.uniqueNameIndex;
    return result;
}

void requireNoNarrowing(const Actor &source, const Actor &restored) {
    const std::string slot = " at actor slot " + std::to_string(source.id);
    if (source.pos != restored.pos)
        throw std::runtime_error("MM6 ACTR position narrows" + slot);
    if (source.velocity != restored.velocity)
        throw std::runtime_error("MM6 ACTR velocity narrows" + slot);
    if (source.initialPosition != restored.initialPosition)
        throw std::runtime_error("MM6 ACTR initial position narrows" + slot);
    if (source.guardingPosition != restored.guardingPosition)
        throw std::runtime_error("MM6 ACTR guarding position narrows" + slot);
    if (source.sectorId != restored.sectorId)
        throw std::runtime_error("MM6 ACTR sector narrows" + slot);
    if (source.currentActionLength != restored.currentActionLength)
        throw std::runtime_error("MM6 ACTR action length narrows" + slot);
    if (source.currentActionTime != restored.currentActionTime)
        throw std::runtime_error("MM6 ACTR action time narrows" + slot);
    if (source.monsterInfo.recoveryTime != restored.monsterInfo.recoveryTime)
        throw std::runtime_error("MM6 ACTR recovery time narrows" + slot);
    if (source.hostilityGroup != restored.hostilityGroup)
        throw std::runtime_error("MM6 ACTR hostility group narrows" + slot);
    for (size_t i = 0; i < source.scheduledJobs.size(); ++i)
        if (source.scheduledJobs[i].vPos != restored.scheduledJobs[i].vPos)
            throw std::runtime_error("MM6 ACTR job position narrows" + slot);
}

void requireSnapshotRoundTrip(const Actor &source, const Actor_MM7 &wire) {
    Actor restored(source.id);
    reconstruct(wire, &restored);
    restored.monsterInfo.mm6Resistances = source.monsterInfo.mm6Resistances;
    restored.donebloodsplat = source.donebloodsplat;
    restored.massDistortionTime = source.massDistortionTime;
    requireNoNarrowing(source, restored);
    const Actor_MM7 repeat = snapshotWithoutName(restored);
    if (std::memcmp(&wire, &repeat, sizeof(wire)) != 0)
        throw std::runtime_error("MM6 spawned actor snapshot does not replay exactly");
}

} // namespace

Actor_MM7 snapshotActorRuntimeImageForV5(const Actor &actor,
                                         std::size_t slot, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 ARTR image requires MM6 title");
    requireSupportedActor(actor, slot);
    if (!actor.monsterInfo.name.empty() ||
        !actor.monsterInfo.internalName.empty())
        throw std::runtime_error("MM6 ARTR cannot store actor runtime names");
    return snapshotWithoutName(actor);
}

Actor_MM7 snapshotActorImageWithoutName(const Actor &actor) {
    return snapshotWithoutName(actor);
}

std::vector<std::uint8_t> encodeSpawnedActorSection(
    std::span<const Actor> actors, size_t firstSlot, GameTitle title) {
    if (title != GameTitle::MM6 || firstSlot > MAX_ACTOR_SLOTS ||
        actors.size() > MAX_ACTOR_SLOTS - firstSlot ||
        actors.size() > (MAX_SECTION_BYTES - HEADER_BYTES) / RECORD_BYTES)
        throw std::runtime_error("MM6 spawned actor section has invalid title or slot count");
    std::vector<std::uint8_t> bytes(HEADER_BYTES + actors.size() * RECORD_BYTES);
    std::copy_n("SPAW", 4, bytes.begin());
    bytes[4] = 1;
    write32(bytes, 8, static_cast<std::uint32_t>(firstSlot));
    write32(bytes, 12, static_cast<std::uint32_t>(actors.size()));
    for (size_t i = 0; i < actors.size(); ++i) {
        const Actor &actor = actors[i];
        requireSupportedActor(actor, firstSlot + i);
        const Actor_MM7 wire = snapshotWithoutName(actor);
        requireSnapshotRoundTrip(actor, wire);
        auto record = std::span(bytes).subspan(HEADER_BYTES + i * RECORD_BYTES, RECORD_BYTES);
        std::memcpy(record.data(), &wire, sizeof(wire));
        const MonsterResistances &res = *actor.monsterInfo.mm6Resistances;
        const std::array<std::uint8_t, 6> resistanceBytes = {
            res.fire, res.electricity, res.cold, res.poison, res.physical, res.magic};
        std::copy(resistanceBytes.begin(), resistanceBytes.end(), record.begin() + sizeof(wire));
        record[sizeof(wire) + 6] = actor.donebloodsplat;
        write64(record, sizeof(wire) + 8, static_cast<std::uint64_t>(actor.massDistortionTime.ticks()));
    }
    return bytes;
}

std::vector<Actor> decodeSpawnedActorSection(
    std::span<const std::uint8_t> bytes, size_t expectedFirstSlot, GameTitle title) {
    if (title != GameTitle::MM6 || bytes.size() < HEADER_BYTES ||
        bytes.size() > MAX_SECTION_BYTES ||
        !std::equal(bytes.begin(), bytes.begin() + 4, "SPAW") ||
        bytes[4] != 1 || bytes[5] != 0 || bytes[6] != 0 || bytes[7] != 0)
        throw std::runtime_error("invalid MM6 spawned actor section header");
    const size_t firstSlot = read32(bytes, 8);
    const size_t count = read32(bytes, 12);
    if (firstSlot != expectedFirstSlot || firstSlot > MAX_ACTOR_SLOTS ||
        count > MAX_ACTOR_SLOTS - firstSlot ||
        count > (MAX_SECTION_BYTES - HEADER_BYTES) / RECORD_BYTES ||
        bytes.size() != HEADER_BYTES + count * RECORD_BYTES)
        throw std::runtime_error("MM6 spawned actor section has invalid slots or size");
    std::vector<Actor> actors;
    actors.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const auto record = bytes.subspan(HEADER_BYTES + i * RECORD_BYTES, RECORD_BYTES);
        Actor_MM7 wire;
        std::memcpy(&wire, record.data(), sizeof(wire));
        if (wire.monsterInfo.id < std::to_underlying(MONSTER_FIRST) ||
            wire.monsterInfo.id > std::to_underlying(MONSTER_LAST) ||
            (wire.monsterId != std::to_underlying(MONSTER_INVALID) &&
             (wire.monsterId < std::to_underlying(MONSTER_FIRST) ||
              wire.monsterId > std::to_underlying(MONSTER_LAST))) ||
            record[sizeof(wire) + 6] > 1 || record[sizeof(wire) + 7] != 0)
            throw std::runtime_error("MM6 spawned actor record has invalid identity or flags");
        requirePristineWireItemsAndBuffs(wire);
        Actor actor(static_cast<int>(firstSlot + i));
        reconstruct(wire, &actor);
        actor.monsterInfo.mm6Resistances = MonsterResistances{
            record[sizeof(wire)], record[sizeof(wire) + 1], record[sizeof(wire) + 2],
            record[sizeof(wire) + 3], record[sizeof(wire) + 4], record[sizeof(wire) + 5]};
        actor.donebloodsplat = record[sizeof(wire) + 6];
        actor.massDistortionTime = Duration::fromTicks(static_cast<std::int64_t>(read64(record, sizeof(wire) + 8)));
        requireSupportedActor(actor, firstSlot + i);
        const Actor_MM7 repeat = snapshotWithoutName(actor);
        if (std::memcmp(&wire, &repeat, sizeof(wire)) != 0)
            throw std::runtime_error("MM6 spawned actor record does not replay exactly");
        actors.push_back(std::move(actor));
    }
    return actors;
}

} // namespace mm6
