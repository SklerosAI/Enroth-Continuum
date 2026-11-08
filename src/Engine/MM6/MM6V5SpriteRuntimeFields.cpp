#include "MM6V5SpriteRuntimeFields.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "MM6OutdoorWire.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> OUTDOOR_TAG = {'S', 'P', 'R', 'T'};
constexpr std::array<char, 4> INDOOR_TAG = {'I', 'S', 'R', 'T'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t MAX_SPRITES = 4096;
constexpr std::size_t SNAPSHOT_BYTES = sizeof(SpriteObject_MM7);
constexpr std::size_t RECORD_BYTES = 100 + SNAPSHOT_BYTES + 9 * 4 + 4 * 8 + 4 + 1 + 28;
constexpr std::size_t INDOOR_ITEM_BYTES = 9 * 4 + 8;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 SPRT requires MM6 title");
}

void requireMapName(std::string_view name, SpriteRuntimeSectionKind kind) {
    const std::string_view suffix = kind == SpriteRuntimeSectionKind::OutdoorV5 ?
        ".odm" : ".blv";
    if (name.empty() || name.size() > 63 || !name.ends_with(suffix) ||
        !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '.';
        }))
        throw std::runtime_error("MM6 SPRT map name is not canonical outdoor identity");
}

void put32(std::vector<std::uint8_t> *out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(value >> shift);
}

void put64(std::vector<std::uint8_t> *out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        out->push_back(value >> shift);
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 SPRT section");
        return _bytes[_offset++];
    }

    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }

    std::uint64_t u64() {
        std::uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 8)
            result |= std::uint64_t(u8()) << shift;
        return result;
    }

    std::span<const std::uint8_t> take(std::size_t size) {
        if (size > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 SPRT record");
        const auto result = _bytes.subspan(_offset, size);
        _offset += size;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("MM6 SPRT has trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

void requireFinite(const Vec3f &value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
        !std::isfinite(value.z) ||
        std::abs(value.x) > 32767 || std::abs(value.y) > 32767 ||
        std::abs(value.z) > 32767)
        throw std::runtime_error("MM6 SPRT geometry cannot safely reach native snapshot");
}

bool sameFloat(float lhs, float rhs) {
    return std::bit_cast<std::uint32_t>(lhs) == std::bit_cast<std::uint32_t>(rhs);
}

bool sameVec(const Vec3f &lhs, const Vec3f &rhs) {
    return sameFloat(lhs.x, rhs.x) && sameFloat(lhs.y, rhs.y) &&
           sameFloat(lhs.z, rhs.z);
}

bool sameItem(const Item &lhs, const Item &rhs) {
    return lhs.itemId == rhs.itemId && lhs.potionPower == rhs.potionPower &&
        lhs.goldAmount == rhs.goldAmount &&
        lhs.standardEnchantment == rhs.standardEnchantment &&
        lhs.standardEnchantmentStrength == rhs.standardEnchantmentStrength &&
        lhs.specialEnchantment == rhs.specialEnchantment &&
        lhs.numCharges == rhs.numCharges && lhs.maxCharges == rhs.maxCharges &&
        lhs.flags == rhs.flags &&
        lhs.lichJarCharacterIndex == rhs.lichJarCharacterIndex &&
        lhs.enchantmentExpirationTime.ticks() == rhs.enchantmentExpirationTime.ticks() &&
        lhs.mm6RawRecord == rhs.mm6RawRecord;
}

std::string_view firstItemMismatch(const Item &lhs, const Item &rhs) {
    if (lhs.itemId != rhs.itemId) return "ID";
    if (lhs.potionPower != rhs.potionPower) return "potion power";
    if (lhs.goldAmount != rhs.goldAmount) return "gold amount";
    if (lhs.standardEnchantment != rhs.standardEnchantment)
        return "standard enchantment";
    if (lhs.standardEnchantmentStrength != rhs.standardEnchantmentStrength)
        return "standard enchantment strength";
    if (lhs.specialEnchantment != rhs.specialEnchantment)
        return "special enchantment";
    if (lhs.numCharges != rhs.numCharges) return "charges";
    if (lhs.maxCharges != rhs.maxCharges) return "maximum charges";
    if (lhs.flags != rhs.flags) return "flags";
    if (lhs.lichJarCharacterIndex != rhs.lichJarCharacterIndex)
        return "jar character index";
    if (lhs.enchantmentExpirationTime.ticks() !=
        rhs.enchantmentExpirationTime.ticks()) return "enchantment expiration";
    if (lhs.mm6RawRecord != rhs.mm6RawRecord) return "MM6 raw record";
    return "unknown field";
}

bool sameSprite(const SpriteObject &lhs, const SpriteObject &rhs) {
    return lhs.spriteId == rhs.spriteId &&
        lhs.uObjectDescID == rhs.uObjectDescID &&
        sameVec(lhs.vPosition, rhs.vPosition) &&
        sameVec(lhs.vVelocity, rhs.vVelocity) &&
        lhs.uFacing == rhs.uFacing && lhs.uAttributes == rhs.uAttributes &&
        lhs.uSectorID == rhs.uSectorID &&
        lhs.timeSinceCreated.ticks() == rhs.timeSinceCreated.ticks() &&
        lhs.tempLifetime.ticks() == rhs.tempLifetime.ticks() &&
        lhs.field_22_glow_radius_multiplier == rhs.field_22_glow_radius_multiplier &&
        sameItem(lhs.containing_item, rhs.containing_item) &&
        lhs.uSpellID == rhs.uSpellID && lhs.spell_level == rhs.spell_level &&
        lhs.spell_skill == rhs.spell_skill && lhs.field_54 == rhs.field_54 &&
        lhs.spell_caster_pid.packed() == rhs.spell_caster_pid.packed() &&
        lhs.spell_target_pid.packed() == rhs.spell_target_pid.packed() &&
        lhs.field_60_distance_related_prolly_lod ==
            rhs.field_60_distance_related_prolly_lod &&
        lhs.spellCasterAbility == rhs.spellCasterAbility &&
        sameVec(lhs.initialPosition, rhs.initialPosition) &&
        lhs._lastParticleTime.ticks() == rhs._lastParticleTime.ticks() &&
        lhs._ticksPerParticle.ticks() == rhs._ticksPerParticle.ticks();
}

std::string firstSpriteMismatch(const SpriteObject &lhs,
                                const SpriteObject &rhs) {
    if (lhs.spriteId != rhs.spriteId) return "sprite ID";
    if (lhs.uObjectDescID != rhs.uObjectDescID) return "object descriptor";
    if (!sameVec(lhs.vPosition, rhs.vPosition)) return "position";
    if (!sameVec(lhs.vVelocity, rhs.vVelocity)) return "velocity";
    if (lhs.uFacing != rhs.uFacing) return "facing";
    if (lhs.uAttributes != rhs.uAttributes) return "attributes";
    if (lhs.uSectorID != rhs.uSectorID) return "sector";
    if (lhs.timeSinceCreated.ticks() != rhs.timeSinceCreated.ticks()) return "age";
    if (lhs.tempLifetime.ticks() != rhs.tempLifetime.ticks()) return "lifetime";
    if (lhs.field_22_glow_radius_multiplier != rhs.field_22_glow_radius_multiplier)
        return "glow radius";
    if (!sameItem(lhs.containing_item, rhs.containing_item))
        return std::string("contained item ") +
               std::string(firstItemMismatch(lhs.containing_item, rhs.containing_item));
    if (lhs.uSpellID != rhs.uSpellID) return "spell ID";
    if (lhs.spell_level != rhs.spell_level) return "spell level";
    if (lhs.spell_skill != rhs.spell_skill) return "spell skill";
    if (lhs.field_54 != rhs.field_54) return "field 54";
    if (lhs.spell_caster_pid.packed() != rhs.spell_caster_pid.packed())
        return "caster PID";
    if (lhs.spell_target_pid.packed() != rhs.spell_target_pid.packed())
        return "target PID";
    if (lhs.field_60_distance_related_prolly_lod !=
        rhs.field_60_distance_related_prolly_lod) return "field 60";
    if (lhs.spellCasterAbility != rhs.spellCasterAbility) return "caster ability";
    if (!sameVec(lhs.initialPosition, rhs.initialPosition)) return "initial position";
    if (lhs._lastParticleTime.ticks() != rhs._lastParticleTime.ticks())
        return "last particle time";
    if (lhs._ticksPerParticle.ticks() != rhs._ticksPerParticle.ticks())
        return "particle period";
    return "unknown field";
}

enum class AnchorPolicy { Strict, SyntheticAppend, ReusedFreeSlot };

void requireNativeAnchor(const SpriteRuntimeRecord &record,
                         AnchorPolicy policy = AnchorPolicy::Strict) {
    requireFinite(record.runtime.vPosition);
    requireFinite(record.runtime.vVelocity);
    requireFinite(record.runtime.initialPosition);
    if (std::to_underlying(record.runtime.containing_item.itemId) < 0 ||
        std::to_underlying(record.runtime.containing_item.itemId) > 580)
        throw std::runtime_error("MM6 SPRT item ID is outside title table");
    if (policy == AnchorPolicy::ReusedFreeSlot)
        return;
    const SpriteObject_MM7 native = projectMM6GroundItemRecord(record.nativeRecord);
    if (native.spriteId != std::to_underlying(record.runtime.spriteId))
        throw std::runtime_error("MM6 SPRT native sprite ID changed");
    if (native.objectDescId != record.runtime.uObjectDescID)
        throw std::runtime_error("MM6 SPRT native object descriptor changed");
    if (policy == AnchorPolicy::Strict &&
        native.initialPosition.toFloat() != record.runtime.initialPosition)
        throw std::runtime_error("MM6 SPRT native initial position changed");
    SpriteObject_MM7 projected{};
    snapshot(record.runtime, &projected);
    if (std::memcmp(&projected.containing_item,
                    record.nativeRecord.data() + 36, 24) != 0)
        throw std::runtime_error("MM6 SPRT item differs from native raw provenance");
    if (record.runtime.containing_item.mm6RawRecord &&
        !std::equal(record.runtime.containing_item.mm6RawRecord->begin(),
                    record.runtime.containing_item.mm6RawRecord->end(),
                    record.nativeRecord.begin() + 36))
        throw std::runtime_error("MM6 SPRT item raw provenance changed");
}

void putVec(std::vector<std::uint8_t> *out, const Vec3f &value) {
    requireFinite(value);
    put32(out, std::bit_cast<std::uint32_t>(value.x));
    put32(out, std::bit_cast<std::uint32_t>(value.y));
    put32(out, std::bit_cast<std::uint32_t>(value.z));
}

Vec3f getVec(Reader *input) {
    const Vec3f result(std::bit_cast<float>(input->u32()),
                       std::bit_cast<float>(input->u32()),
                       std::bit_cast<float>(input->u32()));
    requireFinite(result);
    return result;
}

void copyExactItemScalars(const Item &source, Item *target) {
    target->potionPower = source.potionPower;
    target->goldAmount = source.goldAmount;
    target->standardEnchantment = source.standardEnchantment;
    target->standardEnchantmentStrength = source.standardEnchantmentStrength;
    target->specialEnchantment = source.specialEnchantment;
    target->numCharges = source.numCharges;
    target->maxCharges = source.maxCharges;
    target->flags = source.flags;
    target->lichJarCharacterIndex = source.lichJarCharacterIndex;
    target->enchantmentExpirationTime = source.enchantmentExpirationTime;
}

void putExactIndoorItem(std::vector<std::uint8_t> *out, const Item &item) {
    put32(out, std::bit_cast<std::uint32_t>(item.potionPower));
    put32(out, std::bit_cast<std::uint32_t>(item.goldAmount));
    put32(out, std::bit_cast<std::uint32_t>(item.standardEnchantment ?
        static_cast<std::int32_t>(std::to_underlying(*item.standardEnchantment)) : -1));
    put32(out, std::bit_cast<std::uint32_t>(item.standardEnchantmentStrength));
    put32(out, std::to_underlying(item.specialEnchantment));
    put32(out, std::bit_cast<std::uint32_t>(item.numCharges));
    put32(out, std::bit_cast<std::uint32_t>(item.maxCharges));
    put32(out, std::to_underlying(item.flags));
    put32(out, std::bit_cast<std::uint32_t>(item.lichJarCharacterIndex));
    put64(out, std::bit_cast<std::uint64_t>(item.enchantmentExpirationTime.ticks()));
}

void getExactIndoorItem(Reader *input, Item *item) {
    item->potionPower = std::bit_cast<std::int32_t>(input->u32());
    item->goldAmount = std::bit_cast<std::int32_t>(input->u32());
    const auto attribute = std::bit_cast<std::int32_t>(input->u32());
    item->standardEnchantment = attribute == -1 ?
        std::optional<Attribute>{} : std::optional<Attribute>(static_cast<Attribute>(attribute));
    item->standardEnchantmentStrength = std::bit_cast<std::int32_t>(input->u32());
    item->specialEnchantment = static_cast<ItemEnchantment>(input->u32());
    item->numCharges = std::bit_cast<std::int32_t>(input->u32());
    item->maxCharges = std::bit_cast<std::int32_t>(input->u32());
    item->flags = ItemFlags(input->u32());
    item->lichJarCharacterIndex = std::bit_cast<std::int32_t>(input->u32());
    item->enchantmentExpirationTime = Time::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
}

void putRecord(std::vector<std::uint8_t> *out,
               const SpriteRuntimeRecord &record, AnchorPolicy policy,
               bool indoor) {
    requireNativeAnchor(record, policy);
    requireFinite(record.runtime.vPosition);
    requireFinite(record.runtime.vVelocity);
    requireFinite(record.runtime.initialPosition);
    SpriteObject_MM7 snapshotValue{};
    snapshot(record.runtime, &snapshotValue);
    SpriteObject restored;
    reconstruct(snapshotValue, &restored);
    restored.vPosition = record.runtime.vPosition;
    restored.vVelocity = record.runtime.vVelocity;
    restored.initialPosition = record.runtime.initialPosition;
    restored.uSectorID = record.runtime.uSectorID;
    restored.timeSinceCreated = record.runtime.timeSinceCreated;
    restored.tempLifetime = record.runtime.tempLifetime;
    restored._lastParticleTime = record.runtime._lastParticleTime;
    restored._ticksPerParticle = record.runtime._ticksPerParticle;
    restored.containing_item.mm6RawRecord = record.runtime.containing_item.mm6RawRecord;
    if (indoor)
        copyExactItemScalars(record.runtime.containing_item, &restored.containing_item);
    if (!sameSprite(restored, record.runtime))
        throw std::runtime_error("MM6 SPRT runtime " +
                                 std::string(firstSpriteMismatch(restored, record.runtime)) +
                                 " cannot round-trip");
    out->insert(out->end(), record.nativeRecord.begin(), record.nativeRecord.end());
    const auto *start = reinterpret_cast<const std::uint8_t *>(&snapshotValue);
    out->insert(out->end(), start, start + SNAPSHOT_BYTES);
    putVec(out, record.runtime.vPosition);
    putVec(out, record.runtime.vVelocity);
    putVec(out, record.runtime.initialPosition);
    put64(out, std::bit_cast<std::uint64_t>(record.runtime.timeSinceCreated.ticks()));
    put64(out, std::bit_cast<std::uint64_t>(record.runtime.tempLifetime.ticks()));
    put64(out, std::bit_cast<std::uint64_t>(record.runtime._lastParticleTime.ticks()));
    put64(out, std::bit_cast<std::uint64_t>(record.runtime._ticksPerParticle.ticks()));
    put32(out, std::bit_cast<std::uint32_t>(record.runtime.uSectorID));
    out->push_back(record.runtime.containing_item.mm6RawRecord ? 1 : 0);
    if (record.runtime.containing_item.mm6RawRecord)
        out->insert(out->end(), record.runtime.containing_item.mm6RawRecord->begin(),
                    record.runtime.containing_item.mm6RawRecord->end());
    else
        out->insert(out->end(), 28, 0);
    if (indoor)
        putExactIndoorItem(out, record.runtime.containing_item);
}

SpriteRuntimeRecord getRecord(Reader *input, bool indoor) {
    SpriteRuntimeRecord result;
    const auto native = input->take(100);
    std::copy(native.begin(), native.end(), result.nativeRecord.begin());
    SpriteObject_MM7 snapshotValue{};
    const auto image = input->take(SNAPSHOT_BYTES);
    std::memcpy(&snapshotValue, image.data(), image.size());
    if (snapshotValue.containing_item.itemId < 0 ||
        snapshotValue.containing_item.itemId > 580)
        throw std::runtime_error("MM6 SPRT encoded item ID is outside title table");
    reconstruct(snapshotValue, &result.runtime);
    result.runtime.vPosition = getVec(input);
    result.runtime.vVelocity = getVec(input);
    result.runtime.initialPosition = getVec(input);
    result.runtime.timeSinceCreated = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.runtime.tempLifetime = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.runtime._lastParticleTime = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.runtime._ticksPerParticle = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.runtime.uSectorID = std::bit_cast<std::int32_t>(input->u32());
    const std::uint8_t hasRaw = input->u8();
    const auto raw = input->take(28);
    if (hasRaw > 1 || (!hasRaw && std::any_of(raw.begin(), raw.end(),
                                               [](std::uint8_t value) { return value != 0; })))
        throw std::runtime_error("MM6 SPRT item provenance flag is noncanonical");
    if (hasRaw) {
        ItemRecord28 exact;
        std::copy(raw.begin(), raw.end(), exact.begin());
        result.runtime.containing_item.mm6RawRecord = exact;
    }
    if (indoor)
        getExactIndoorItem(input, &result.runtime.containing_item);
    return result;
}

} // namespace

SpriteRuntimeSaveFields captureSpriteRuntimeFields(
    std::string_view mapName, std::span<const std::uint8_t> nativeDelta,
    std::span<const SpriteObject> live, GameTitle title) {
    requireMM6(title);
    requireMapName(mapName, SpriteRuntimeSectionKind::OutdoorV5);
    const auto wire = decodeOutdoorDelta(nativeDelta);
    if (live.size() != wire.groundItemCount || live.size() > MAX_SPRITES)
        throw std::runtime_error("MM6 SPRT live slot count differs from native DDM");
    SpriteRuntimeSaveFields result;
    result.mapName = mapName;
    const auto ground = wire.part(OutdoorDeltaPart::GroundItems).subspan(4);
    result.records.reserve(live.size());
    for (std::size_t index = 0; index < live.size(); ++index) {
        SpriteRuntimeRecord record;
        std::copy_n(ground.begin() + index * 100, 100, record.nativeRecord.begin());
        record.runtime = live[index];
        requireNativeAnchor(record);
        result.records.push_back(std::move(record));
    }
    (void) encodeSpriteRuntimeSection(result, title);
    return result;
}

SaveSection encodeSpriteRuntimeSection(const SpriteRuntimeSaveFields &fields,
                                       GameTitle title) {
    return encodeSpriteRuntimeSectionAs(fields, SpriteRuntimeSectionKind::OutdoorV5, title);
}

SaveSection encodeSpriteRuntimeSectionAs(const SpriteRuntimeSaveFields &fields,
                                         SpriteRuntimeSectionKind kind,
                                         GameTitle title) {
    requireMM6(title);
    requireMapName(fields.mapName, kind);
    if (fields.records.size() > MAX_SPRITES ||
        (kind == SpriteRuntimeSectionKind::OutdoorV5 && fields.nativePrefixCount) ||
        (kind == SpriteRuntimeSectionKind::IndoorV6 &&
         fields.nativePrefixCount > fields.records.size()))
        throw std::runtime_error("MM6 SPRT exceeds bounded full-list count");
    std::vector<std::uint8_t> bytes;
    const std::size_t extra = kind == SpriteRuntimeSectionKind::IndoorV6 ? 4 : 0;
    bytes.reserve(2 + fields.mapName.size() + 4 + extra +
                  fields.records.size() * (RECORD_BYTES +
                  (kind == SpriteRuntimeSectionKind::IndoorV6 ? INDOOR_ITEM_BYTES : 0)));
    bytes.push_back(VERSION);
    bytes.push_back(fields.mapName.size());
    bytes.insert(bytes.end(), fields.mapName.begin(), fields.mapName.end());
    put32(&bytes, fields.records.size());
    if (kind == SpriteRuntimeSectionKind::IndoorV6)
        put32(&bytes, fields.nativePrefixCount);
    for (std::size_t index = 0; index < fields.records.size(); ++index) {
        try {
            AnchorPolicy policy = AnchorPolicy::Strict;
            if (kind == SpriteRuntimeSectionKind::IndoorV6) {
                if (index >= fields.nativePrefixCount)
                    policy = AnchorPolicy::SyntheticAppend;
                else if (projectMM6GroundItemRecord(
                    fields.records[index].nativeRecord).objectDescId == 0)
                    policy = AnchorPolicy::ReusedFreeSlot;
            }
            putRecord(&bytes, fields.records[index], policy,
                      kind == SpriteRuntimeSectionKind::IndoorV6);
        } catch (const std::exception &error) {
            if (kind == SpriteRuntimeSectionKind::IndoorV6)
                throw std::runtime_error("MM6 ISRT " +
                    std::string(index < fields.nativePrefixCount ? "native" : "appended") +
                    " sprite slot " + std::to_string(index) + ": " + error.what());
            throw;
        }
    }
    return {kind == SpriteRuntimeSectionKind::OutdoorV5 ? OUTDOOR_TAG : INDOOR_TAG,
            true, std::move(bytes)};
}

SpriteRuntimeSaveFields decodeSpriteRuntimeSection(const SaveContainer &container,
                                                   GameTitle title) {
    return decodeSpriteRuntimeSectionAs(container, SpriteRuntimeSectionKind::OutdoorV5,
                                        title);
}

SpriteRuntimeSaveFields decodeSpriteRuntimeSectionAs(
    const SaveContainer &container, SpriteRuntimeSectionKind kind,
    GameTitle title) {
    requireMM6(title);
    const auto &tag = kind == SpriteRuntimeSectionKind::OutdoorV5 ?
        OUTDOOR_TAG : INDOOR_TAG;
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections)
        if (candidate.tag == tag) {
            if (section)
                throw std::runtime_error("duplicate MM6 SPRT section");
            section = &candidate;
        }
    if (!section || !section->required || section->payload.size() < 6 ||
        section->payload.size() >
            2 + 63 + 8 + MAX_SPRITES * (RECORD_BYTES + INDOOR_ITEM_BYTES))
        throw std::runtime_error("missing or unbounded MM6 SPRT section");
    Reader input(section->payload);
    if (input.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 SPRT version");
    const auto name = input.take(input.u8());
    SpriteRuntimeSaveFields result;
    result.mapName.assign(name.begin(), name.end());
    requireMapName(result.mapName, kind);
    const std::uint32_t count = input.u32();
    if (kind == SpriteRuntimeSectionKind::IndoorV6)
        result.nativePrefixCount = input.u32();
    const std::size_t extra = kind == SpriteRuntimeSectionKind::IndoorV6 ? 4 : 0;
    const std::size_t recordBytes = RECORD_BYTES +
        (kind == SpriteRuntimeSectionKind::IndoorV6 ? INDOOR_ITEM_BYTES : 0);
    if (count > MAX_SPRITES || result.nativePrefixCount > count ||
        section->payload.size() != 2 + name.size() + 4 + extra + count * recordBytes)
        throw std::runtime_error("MM6 SPRT count differs from payload");
    result.records.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
        result.records.push_back(getRecord(
            &input, kind == SpriteRuntimeSectionKind::IndoorV6));
    input.finish();
    if (encodeSpriteRuntimeSectionAs(result, kind, title) != *section)
        throw std::runtime_error("MM6 SPRT is not canonical");
    return result;
}

void requireSpriteRuntimeNativeAgreement(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    std::span<const std::uint8_t> nativeDelta, GameTitle title) {
    requireMM6(title);
    if (fields.mapName != mapName)
        throw std::runtime_error("MM6 SPRT map differs from HEAD");
    const auto wire = decodeOutdoorDelta(nativeDelta);
    if (fields.records.size() != wire.groundItemCount)
        throw std::runtime_error("MM6 SPRT slot count differs from native DDM");
    const auto ground = wire.part(OutdoorDeltaPart::GroundItems).subspan(4);
    for (std::size_t index = 0; index < fields.records.size(); ++index) {
        if (!std::equal(fields.records[index].nativeRecord.begin(),
                        fields.records[index].nativeRecord.end(),
                        ground.begin() + index * 100))
            throw std::runtime_error("MM6 SPRT native slot anchor differs");
        requireNativeAnchor(fields.records[index]);
    }
}

std::vector<SpriteObject> stageSpriteRuntimeReplay(
    const SpriteRuntimeSaveFields &fields, std::span<const SpriteObject> rebuilt,
    std::string_view mapName, std::span<const std::uint8_t> nativeDelta, GameTitle title) {
    if (rebuilt.size() != fields.records.size())
        throw std::runtime_error("MM6 v5 sprite replay lacks an exact native map context");
    requireSpriteRuntimeNativeAgreement(fields, mapName, nativeDelta, title);
    const OutdoorDeltaWire wire = decodeOutdoorDelta(nativeDelta);
    const auto ground = wire.part(OutdoorDeltaPart::GroundItems).subspan(4);

    std::vector<SpriteObject> saved;
    saved.reserve(fields.records.size());
    for (std::size_t slot = 0; slot < fields.records.size(); ++slot) {
        const auto native = projectMM6GroundItemRecord(ground.subspan(slot * 100, 100));
        const SpriteObject &restored = fields.records[slot].runtime;
        const auto sameAnchor = [&](const SpriteObject &sprite) {
            return std::to_underlying(sprite.spriteId) == native.spriteId &&
                sprite.uObjectDescID == native.objectDescId &&
                sprite.initialPosition == native.initialPosition.toFloat();
        };
        if (!sameAnchor(rebuilt[slot]) || !sameAnchor(restored))
            throw std::runtime_error("MM6 v5 sprite PID anchor differs from native DDM");
        saved.push_back(restored);
    }
    return saved;
}

} // namespace mm6
