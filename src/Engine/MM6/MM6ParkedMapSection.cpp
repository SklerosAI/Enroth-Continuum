#include "MM6ParkedMapSection.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Snapshots/EntitySnapshots.h"

#include "MM6SpawnedActorSection.h"

namespace mm6 {

static constexpr std::array<char, 4> PRKM = {'P', 'R', 'K', 'M'};
static constexpr std::uint8_t VERSION = 1;
static constexpr std::size_t MAX_MAPS = 67;
static constexpr std::size_t MAX_ACTORS = 8192;
static constexpr std::size_t MAX_SPRITES = 4096;
static constexpr std::size_t MAX_PAYLOAD = 32 * 1024 * 1024; // Section size bound of the EC6 container.

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 PRKM requires the MM6 title");
}

static bool isCanonicalMapName(std::string_view name) {
    return name.size() > 4 && name.size() <= 63 && (name.ends_with(".odm") || name.ends_with(".blv")) &&
           std::ranges::all_of(name, [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.'; });
}

class ParkedMapWriter {
 public:
    void u8(std::uint8_t value) {
        _bytes.push_back(value);
    }

    void u32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8)
            _bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }

    void u64(std::uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8)
            _bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }

    void i32(std::int32_t value) {
        u32(std::bit_cast<std::uint32_t>(value));
    }

    void i64(std::int64_t value) {
        u64(std::bit_cast<std::uint64_t>(value));
    }

    void vec(const Vec3f &value) {
        if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
            throw std::runtime_error("MM6 PRKM position is not finite");
        u32(std::bit_cast<std::uint32_t>(value.x));
        u32(std::bit_cast<std::uint32_t>(value.y));
        u32(std::bit_cast<std::uint32_t>(value.z));
    }

    void bytes(std::span<const std::uint8_t> value) {
        _bytes.insert(_bytes.end(), value.begin(), value.end());
        if (_bytes.size() > MAX_PAYLOAD)
            throw std::runtime_error("MM6 PRKM exceeds the section bound");
    }

    template<class T>
    void image(const T &value) {
        bytes(std::span(reinterpret_cast<const std::uint8_t *>(&value), sizeof(value)));
    }

    std::vector<std::uint8_t> take() {
        return std::move(_bytes);
    }

 private:
    std::vector<std::uint8_t> _bytes;
};

class ParkedMapReader {
 public:
    explicit ParkedMapReader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::span<const std::uint8_t> bytes(std::size_t count) {
        if (count > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 PRKM section");
        const auto result = _bytes.subspan(_offset, count);
        _offset += count;
        return result;
    }

    std::uint8_t u8() {
        return bytes(1)[0];
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

    std::int32_t i32() {
        return std::bit_cast<std::int32_t>(u32());
    }

    std::int64_t i64() {
        return std::bit_cast<std::int64_t>(u64());
    }

    Vec3f vec() {
        const float x = std::bit_cast<float>(u32());
        const float y = std::bit_cast<float>(u32());
        const float z = std::bit_cast<float>(u32());
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
            throw std::runtime_error("MM6 PRKM position is not finite");
        return Vec3f(x, y, z);
    }

    template<class T>
    T image() {
        T result;
        const auto source = bytes(sizeof(T));
        std::memcpy(&result, source.data(), sizeof(T));
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("MM6 PRKM has trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

static void putItem(ParkedMapWriter *out, const Item &item) {
    out->i32(std::to_underlying(item.itemId));
    out->i32(item.potionPower);
    out->i32(item.goldAmount);
    out->i32(item.standardEnchantment ? std::to_underlying(*item.standardEnchantment) : -1);
    out->i32(item.standardEnchantmentStrength);
    out->i32(std::to_underlying(item.specialEnchantment));
    out->i32(item.numCharges);
    out->i32(item.maxCharges);
    out->u32(std::to_underlying(item.flags));
    out->i32(item.lichJarCharacterIndex);
    out->i64(item.enchantmentExpirationTime.ticks());
    out->u8(item.mm6RawRecord ? 1 : 0);
    if (item.mm6RawRecord)
        out->image(*item.mm6RawRecord);
}

static Item getItem(ParkedMapReader *in) {
    Item item;
    item.itemId = static_cast<ItemId>(in->i32());
    item.potionPower = in->i32();
    item.goldAmount = in->i32();
    const std::int32_t attribute = in->i32();
    if (attribute < -1)
        throw std::runtime_error("MM6 PRKM item enchantment is out of range");
    if (attribute >= 0)
        item.standardEnchantment = static_cast<Attribute>(attribute);
    item.standardEnchantmentStrength = in->i32();
    item.specialEnchantment = static_cast<ItemEnchantment>(in->i32());
    item.numCharges = in->i32();
    item.maxCharges = in->i32();
    item.flags = ItemFlags(in->u32());
    item.lichJarCharacterIndex = in->i32();
    item.enchantmentExpirationTime = Time::fromTicks(in->i64());
    const std::uint8_t hasRaw = in->u8();
    if (hasRaw > 1)
        throw std::runtime_error("MM6 PRKM item record flag is not canonical");
    if (hasRaw)
        item.mm6RawRecord = in->image<ItemRecord28>();
    return item;
}

static void putActor(ParkedMapWriter *out, const Actor &actor, std::size_t slot) {
    if (actor.id != static_cast<int>(slot))
        throw std::runtime_error("MM6 PRKM actor is out of its slot");
    out->image(snapshotActorImageWithoutName(actor));
    out->vec(actor.pos);
    out->vec(actor.velocity);
    out->vec(actor.initialPosition);
    out->vec(actor.guardingPosition);
    for (const ActorJob &job : actor.scheduledJobs)
        out->vec(job.vPos);
    out->i64(actor.currentActionLength.ticks());
    out->i64(actor.currentActionTime.ticks());
    out->i64(actor.monsterInfo.recoveryTime.ticks());
    out->i32(actor.sectorId);
    out->i32(std::to_underlying(actor.hostilityGroup));
    out->u8(actor.monsterInfo.mm6Resistances ? 1 : 0);
    const MonsterResistances resistances = actor.monsterInfo.mm6Resistances.value_or(MonsterResistances());
    for (std::uint8_t value : {resistances.fire, resistances.electricity, resistances.cold, resistances.poison,
                               resistances.physical, resistances.magic})
        out->u8(value);
    out->u8(actor.donebloodsplat ? 1 : 0);
    out->i64(actor.massDistortionTime.ticks());
    for (const Item &item : actor.items)
        putItem(out, item);
}

static Actor getActor(ParkedMapReader *in, std::size_t slot) {
    Actor actor(static_cast<int>(slot));
    reconstruct(in->image<Actor_MM7>(), &actor);
    actor.pos = in->vec();
    actor.velocity = in->vec();
    actor.initialPosition = in->vec();
    actor.guardingPosition = in->vec();
    for (ActorJob &job : actor.scheduledJobs)
        job.vPos = in->vec();
    actor.currentActionLength = Duration::fromTicks(in->i64());
    actor.currentActionTime = Duration::fromTicks(in->i64());
    actor.monsterInfo.recoveryTime = Duration::fromTicks(in->i64());
    actor.sectorId = in->i32();
    actor.hostilityGroup = static_cast<MonsterType>(in->i32());
    const std::uint8_t hasResistances = in->u8();
    MonsterResistances resistances;
    resistances.fire = in->u8();
    resistances.electricity = in->u8();
    resistances.cold = in->u8();
    resistances.poison = in->u8();
    resistances.physical = in->u8();
    resistances.magic = in->u8();
    if (hasResistances > 1)
        throw std::runtime_error("MM6 PRKM actor resistance flag is not canonical");
    if (hasResistances)
        actor.monsterInfo.mm6Resistances = resistances;
    const std::uint8_t blood = in->u8();
    if (blood > 1)
        throw std::runtime_error("MM6 PRKM actor blood flag is not canonical");
    actor.donebloodsplat = blood;
    actor.massDistortionTime = Duration::fromTicks(in->i64());
    for (Item &item : actor.items)
        item = getItem(in);
    return actor;
}

static void putSprite(ParkedMapWriter *out, const SpriteObject &sprite) {
    SpriteObject_MM7 image;
    snapshot(sprite, &image);
    out->image(image);
    out->vec(sprite.vPosition);
    out->vec(sprite.vVelocity);
    out->vec(sprite.initialPosition);
    out->i64(sprite.timeSinceCreated.ticks());
    out->i64(sprite.tempLifetime.ticks());
    out->i64(sprite._lastParticleTime.ticks());
    out->i64(sprite._ticksPerParticle.ticks());
    out->i32(sprite.uSectorID);
    putItem(out, sprite.containing_item);
}

static SpriteObject getSprite(ParkedMapReader *in) {
    SpriteObject sprite;
    reconstruct(in->image<SpriteObject_MM7>(), &sprite);
    sprite.vPosition = in->vec();
    sprite.vVelocity = in->vec();
    sprite.initialPosition = in->vec();
    sprite.timeSinceCreated = Duration::fromTicks(in->i64());
    sprite.tempLifetime = Duration::fromTicks(in->i64());
    sprite._lastParticleTime = Duration::fromTicks(in->i64());
    sprite._ticksPerParticle = Duration::fromTicks(in->i64());
    sprite.uSectorID = in->i32();
    sprite.containing_item = getItem(in);
    return sprite;
}

static std::vector<std::uint8_t> encodePayload(std::span<const ParkedMapRuntime> maps) {
    if (maps.size() > MAX_MAPS)
        throw std::runtime_error("MM6 PRKM has more maps than the title");
    std::set<std::string_view> names;
    ParkedMapWriter out;
    out.u8(VERSION);
    out.u32(maps.size());
    for (const ParkedMapRuntime &map : maps) {
        if (!isCanonicalMapName(map.mapName) || !names.insert(map.mapName).second)
            throw std::runtime_error("MM6 PRKM map name is not canonical or repeats");
        if (map.actors.size() > MAX_ACTORS || map.sprites.size() > MAX_SPRITES)
            throw std::runtime_error("MM6 PRKM map exceeds its list bounds");
        out.u8(map.mapName.size());
        out.bytes(std::span(reinterpret_cast<const std::uint8_t *>(map.mapName.data()), map.mapName.size()));
        out.bytes(map.variables.mapVars);
        out.bytes(map.variables.decorVars);
        out.u32(map.actors.size());
        for (std::size_t slot = 0; slot < map.actors.size(); ++slot)
            putActor(&out, map.actors[slot], slot);
        out.u32(map.sprites.size());
        for (const SpriteObject &sprite : map.sprites)
            putSprite(&out, sprite);
    }
    return out.take();
}

static std::vector<ParkedMapRuntime> decodePayload(std::span<const std::uint8_t> payload) {
    ParkedMapReader in(payload);
    if (in.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 PRKM version");
    const std::uint32_t count = in.u32();
    if (count > MAX_MAPS)
        throw std::runtime_error("MM6 PRKM has more maps than the title");
    std::vector<ParkedMapRuntime> result(count);
    for (ParkedMapRuntime &map : result) {
        const auto name = in.bytes(in.u8());
        map.mapName.assign(name.begin(), name.end());
        const auto mapVars = in.bytes(map.variables.mapVars.size());
        std::ranges::copy(mapVars, map.variables.mapVars.begin());
        const auto decorVars = in.bytes(map.variables.decorVars.size());
        std::ranges::copy(decorVars, map.variables.decorVars.begin());
        const std::uint32_t actorCount = in.u32();
        if (actorCount > MAX_ACTORS)
            throw std::runtime_error("MM6 PRKM map exceeds its list bounds");
        for (std::size_t slot = 0; slot < actorCount; ++slot)
            map.actors.push_back(getActor(&in, slot));
        const std::uint32_t spriteCount = in.u32();
        if (spriteCount > MAX_SPRITES)
            throw std::runtime_error("MM6 PRKM map exceeds its list bounds");
        for (std::size_t slot = 0; slot < spriteCount; ++slot)
            map.sprites.push_back(getSprite(&in));
    }
    in.finish();
    return result;
}

SaveSection encodeParkedMapSection(std::span<const ParkedMapRuntime> maps, GameTitle title) {
    requireMM6(title);
    std::vector<std::uint8_t> payload = encodePayload(maps);
    if (encodePayload(decodePayload(payload)) != payload)
        throw std::runtime_error("MM6 PRKM doesn't round-trip exactly");
    return {PRKM, true, std::move(payload)};
}

std::vector<ParkedMapRuntime> decodeParkedMapSection(const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag != PRKM)
            continue;
        if (section)
            throw std::runtime_error("duplicate MM6 PRKM section");
        section = &candidate;
    }
    if (!section || !section->required)
        throw std::runtime_error("missing MM6 PRKM section");
    std::vector<ParkedMapRuntime> result = decodePayload(section->payload);
    if (encodePayload(result) != section->payload)
        throw std::runtime_error("MM6 PRKM is not canonical");
    return result;
}

} // namespace mm6
